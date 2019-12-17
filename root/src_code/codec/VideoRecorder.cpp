#include "VideoRecorder.h"
#include <chrono>
#include <future>
#include <thread>

namespace duobei::capturer {
VideoRecorder::VideoRecorder(int di,
                             const std::string& dn,
                             const std::string& fn)
    : device_{std::to_string(di), dn, fn} {
#if !defined(FF_API_NEXT)
    //init ffmpeg
    av_register_all();
#endif
    avdevice_register_all();
}

VideoRecorder::~VideoRecorder() {
    if (packet != nullptr) {
        av_packet_free(&packet);
        packet = nullptr;
    }

    if (frame != nullptr) {
        av_frame_free(&frame);
        frame = nullptr;
    }

    if (fmt_ctx != nullptr) {
        avformat_close_input(&fmt_ctx);
        fmt_ctx = nullptr;
    }
#if 0
    if (codec_context_ != nullptr) {
        avcodec_close(codec_context_);
        codec_context_ = nullptr;
    }
#endif
}



int VideoRecorder::SetContextWin() {
    // ffmpeg -f dshow -list_options true -i audio="Microphone (High Definition Audio Device)"
    // ffplay -f dshow -i audio="Microphone (High Definition Audio Device)"

    AVDictionary* options = nullptr;

    av_dict_set_int(&options, "audio_device_number", std::stoi(device_.index), 0);
    av_dict_set_int(&options, "channels", 1, 0);
    av_dict_set_int(&options, "sample_rate", 16000, 0);
    av_dict_set_int(&options, "sample_size", 16, 0);
    av_dict_set_int(&options, "audio_buffer_size", 30, 0);
    av_dict_set_int(&options, "timeout", 1000000, 0);  // 10000000 ms
    // av_dict_set(&options, "rtbufsize", "16M", 0);

    std::string deviceName = FixDeviceName(device_.name);
    int ret =
        avformat_open_input(&fmt_ctx, deviceName.c_str(),
                            av_find_input_format(device_.format.c_str()), &options);
    av_dict_free(&options);

    if (ret < 0) {
        ret = avformat_open_input(&fmt_ctx, deviceName.c_str(),
                                  av_find_input_format(device_.format.c_str()),
                                  nullptr);
    }
    return ret;
}

int VideoRecorder::SetContextMac() {
    // ffplay -f avfoundation -framerate 30 -video_device_index 0 -video_size "1280x720" -pixel_format nv12 -
    // ffmpeg -f avfoundation -list_devices true -i ""
    // format_name avfoundation
    // device_index 0  Soundflower (2ch)

    AVDictionary* options = nullptr;
    av_dict_set(&options, "framerate", "30", 0);
    av_dict_set(&options, "video_device_index", device_.index.c_str(), 0);
    av_dict_set(&options, "video_size", "1280x720", 0);
    av_dict_set(&options, "pixel_format", "yuyv422", 0);

    auto fmt = av_find_input_format(device_.format.c_str());
    assert(fmt);
    auto ret = avformat_open_input(&fmt_ctx, "", fmt, &options);
    return ret;
}

bool VideoRecorder::Open() {
    assert(fmt_ctx == nullptr);
    fmt_ctx = avformat_alloc_context();
    if (fmt_ctx == nullptr) {
        return false;
    }
    // fmt_ctx->interrupt_callback = AVIOInterruptCB{
    // &VideoRecorder::AVIOInterruptCallback, this};
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    auto ret = SetContextWin();
#else
    auto ret = SetContextMac();
#endif
    assert(ret >= 0);

    // WriteDebugLog("fixme: %s, avformat_find_stream_info 阻塞在此处", device_name.c_str());
    ret = avformat_find_stream_info(
        fmt_ctx, nullptr);  // fixme: virtual-audio-capturer 阻塞在此处

    if (ret < 0) {
        return false;
    }

    // select the video stream  判断流是否正常
    AVCodec* dec = nullptr;
    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0) {
        return false;
    }
    stream_index = ret;

    AVStream* st = fmt_ctx->streams[stream_index];
    codec_context_ = avcodec_alloc_context3(dec);
    ret = avcodec_parameters_to_context(codec_context_, st->codecpar);
    if (ret < 0) {
        return false;
    }

    if (auto ret = avcodec_open2(codec_context_, dec, nullptr); ret < 0) {
        return false;
    }
    auto text = device_.name + " [ VideoCapture ] open success";
    printf("%s\n", text.c_str());
    return true;
}

bool VideoRecorder::Read() {
    if (needExit_) {
        return false;
    }

    if (!packet) {
        packet = av_packet_alloc();
    }
    int ret = av_read_frame(fmt_ctx, packet);  // fixme: virtual-audio-capturer 阻塞在此处

    if (ret < 0) {
        return false;
    }

    if (packet->stream_index == stream_index) {
        ret = avcodec_send_packet(codec_context_, packet);
        if (ret < 0) {
            av_packet_unref(packet);
            return false;
        }

        if (frame == nullptr) {
            frame = av_frame_alloc();
        }
        ret = avcodec_receive_frame(codec_context_, frame);
        if (ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            // todo: EAGAIN and AVERROR_EOF
            av_frame_unref(frame);
            return false;
        }
        auto success = Scaling(AV_PIX_FMT_YUV420P);
        if (success) {
            Play(scale_frame);
            av_freep(&scale_frame->data[0]);
        }
        av_frame_unref(frame);
    }
    av_packet_unref(packet);
    return true;
}

void VideoRecorder::Play(AVFrame *frame) {
    int size = av_image_get_buffer_size((AVPixelFormat)frame->format, frame->width, frame->height, 1);
    frame_.Update(frame->data[0], size, frame->width, frame->height, frame->linesize[0]);
    // play_internal->Play((void *)frame->data[0], size, frame->width, frame->height);
}

bool VideoRecorder::Scaling(int pix_fmt) {
    auto codecCtx_ =  codec_context_;
    auto src_frame = frame;
    src_frame->width = codec_context_->width;
    src_frame->height = codec_context_->height;

    auto f = src_frame;

    if (!sws_ctx) {
        sws_ctx = sws_getContext(f->width, f->height, static_cast<AVPixelFormat>(f->format),
                                 f->width, f->height, static_cast<AVPixelFormat>(pix_fmt),
                                 SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!sws_ctx) {
            return false;
        }
    }

    if (scale_frame) {
        av_frame_free(&scale_frame);
        scale_frame = nullptr;
    }
    scale_frame = av_frame_alloc();
    if (!scale_frame) {
        return false;
    }

    scale_frame->format = pix_fmt;
    scale_frame->width = f->width;
    scale_frame->height = f->height;
    auto len = av_image_alloc(scale_frame->data, scale_frame->linesize, scale_frame->width, scale_frame->height,
                              static_cast<AVPixelFormat>(scale_frame->format), 1);

    if (len < 0) {
        return false;
    }

    auto ret = sws_scale(sws_ctx, f->data, f->linesize, 0, scale_frame->height, scale_frame->data,
                         scale_frame->linesize);

    auto success = ret > 0;
    if (success && scale_frame->height > 0 && scale_frame->width > 0) {
        video_size.height = scale_frame->height;
        video_size.width = scale_frame->width;
    }
    return success;
}
}  // namespace duobei::capturer
