#include "H264Decoder.h"
#include "SDLPlayer.h"
#include <cassert>

bool H264Decode::Decode(AVPacket *pkt, uint32_t size) {
    auto ret = context.Send(pkt);
    if (ret < 0) {
        return false;
    }
    if (context.Update()) {
        context.Close();
        context.Open(false, context.parameters);
        ret = context.Send(pkt);
        if (ret < 0) {
            return false;
        }
    }

    while (ret >= 0) {
        int result = context.Receive();
        if (result < 0) {
            return false;
        }

        if (context.src_frame->best_effort_timestamp == AV_NOPTS_VALUE) {
            pts = 0;
        } else {
            context.src_frame->pts = context.src_frame->best_effort_timestamp;
        }

        pts = av_q2d(videoState.stream->time_base) * context.src_frame->pts;
        pts = videoState.synchronize(context.src_frame, pts);

        auto success = context.Scaling(AV_PIX_FMT_YUV420P);
        if (success) {
            playInternal.Play(static_cast<void *>(context.scale_frame->data[0]), size, context.scale_frame->width, context.scale_frame->height, pts);
            av_freep(&context.scale_frame->data[0]);
        }

//        auto that = static_cast<VideoChannel*>(playInternal.handle);
//        if (that->work_queue_.size() >= 30) {
//            std::this_thread::sleep_for(std::chrono::milliseconds(500 * 2));
//        }
//        int size_ = av_image_get_buffer_size((AVPixelFormat)context.src_frame->format, context.src_frame->width, context.src_frame->height, 1);
//        uint8_t *videoBuffer = new uint8_t[size_];
//        av_image_fill_arrays(context.src_frame->data, context.src_frame->linesize,
//                             videoBuffer, static_cast<AVPixelFormat>(context.src_frame->format), context.src_frame->width, context.src_frame->height, 1);
//        playInternal.Play(context.src_frame->data[0], size, context.src_frame->width, context.src_frame->height, pts);
//        delete []videoBuffer;
    }
    return true;
}

bool H264Decode::Context::Open(bool with_hw, const AVCodecParameters* param) {
    // todo: Android 硬解
    // codec = avcodec_find_decoder_by_name("h264_mediacodec");
    if (param) {
        parameters = param;
        codecCtx_ = avcodec_alloc_context3(nullptr);
        assert(avcodec_parameters_to_context(codecCtx_, param) == 0);
        codec = avcodec_find_decoder(codecCtx_->codec_id);
    } else {
        codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        codecCtx_ = avcodec_alloc_context3(codec);
    }
    int ret = avcodec_open2(codecCtx_, codec, nullptr);
    assert(ret == 0);

#if VIDEO_DECODING_USE_HARDWARE
    hw_ctx = new HardwareContext{};
    if (with_hw) {
        hw_ctx->find("videotoolbox", codec);
        //    hw_ctx->reset();
        if (hw_ctx->init(codecCtx_) < 0) {
            return;
        }
    }
#endif
    return true;
}
int H264Decode::Context::Send(const AVPacket *avpkt) {
    int ret = avcodec_send_packet(codecCtx_, avpkt);
    if (ret < 0) {
        printf("Error sending a packet for decoding %d, text %s\n", ret, av_err2str(ret));
    }
    return ret;
}
int H264Decode::Context::Receive() {
    if (src_frame) {
        av_frame_free(&src_frame);
        src_frame = nullptr;
    }
    if (dst_frame) {
        av_frame_free(&dst_frame);
        dst_frame = nullptr;
    }
    src_frame = av_frame_alloc();
    dst_frame = av_frame_alloc();

    if (!src_frame || !dst_frame) {
        return -1;
    }

    int ret = avcodec_receive_frame(codecCtx_, src_frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0) {
        av_frame_free(&src_frame);
        src_frame = nullptr;
        av_frame_free(&dst_frame);
        dst_frame = nullptr;
        if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            printf("Error during decoding %d %s\n", ret, av_err2str(ret));
        }
    }
    return ret;
}
bool H264Decode::Context::Update() const {
    //WriteDebugLog("context.codecCtx_ %dx%d, video_size=%dx%d", codecCtx_->width, codecCtx_->height, video_size.width, video_size.height);
    if (0 == video_size.width || 0 == video_size.height) {
        return false;
    }
    if (0 == codecCtx_->width || 0 == codecCtx_->height) {
        return true;
    }
    if (codecCtx_->width != video_size.width || codecCtx_->height != video_size.height) {
        return true;
    }
    return false;
}
bool H264Decode::Context::Scaling(int pix_fmt) {
    // fix:
    src_frame->width = codecCtx_->width;
    src_frame->height = codecCtx_->height;
#if VIDEO_DECODING_USE_HARDWARE
    auto f = hw_ctx->transfer(dst_frame, src_frame);
    //WriteDebugLog("====== %s -> %s, decoder %s, %dx%d",
    //              av_get_pix_fmt_name(static_cast<AVPixelFormat>(f->format)),
    //              av_get_pix_fmt_name(static_cast<AVPixelFormat>(pix_fmt)),
    //              hw_ctx->hwdevice_name(), src_frame->width, src_frame->height);

    //printf("====== %s -> %s, decoder %s, %dx%d\n",
    //              av_get_pix_fmt_name(static_cast<AVPixelFormat>(f->format)),
    //              av_get_pix_fmt_name(pix_fmt),
    //              hw_ctx->hwdevice_name(), src_frame->width, src_frame->height);
#else
    auto f = src_frame;
#endif


    if (!sws_ctx) {
        sws_ctx = sws_getContext(f->width, f->height, static_cast<AVPixelFormat>(f->format),
                                 f->width, f->height, static_cast<AVPixelFormat>(pix_fmt),
                                 SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!sws_ctx) {
            printf(
                "Impossible to create scale context for the conversion "
                "fmt:%s s:%dx%d -> fmt:%s s:%dx%d\n",
                av_get_pix_fmt_name(codecCtx_->pix_fmt), src_frame->width, src_frame->height,
                av_get_pix_fmt_name(static_cast<AVPixelFormat>(pix_fmt)), src_frame->width, src_frame->height);
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
void H264Decode::Context::Close() {
    if (sws_ctx) {
        sws_freeContext(sws_ctx);
        sws_ctx = nullptr;
    }
    if (src_frame) {
        av_frame_free(&src_frame);
        src_frame = nullptr;
    }
    if (dst_frame) {
        av_frame_free(&dst_frame);
        dst_frame = nullptr;
    }
    if (scale_frame) {
        av_frame_free(&scale_frame);
        scale_frame = nullptr;
    }

    if (codecCtx_) {
        avcodec_close(codecCtx_);
        avcodec_free_context(&codecCtx_);
        codecCtx_ = nullptr;
    }

#if VIDEO_DECODING_USE_HARDWARE
    hw_ctx->reset();
    delete hw_ctx;
#endif
}