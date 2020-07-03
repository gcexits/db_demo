#include "H264Decoder.h"

#include <iostream>

namespace duobei::video {

void Cache::Play(AVFrame *frame, uint32_t timestamp) {
    if (!running) {
        return;
    }
    int size = av_image_get_buffer_size((AVPixelFormat)frame->format, frame->width, frame->height, 1);
    std::lock_guard lck(work_mtx_);
    buffer_ = std::make_unique<Buffer>(frame, size);
#ifdef __APPLE__
    memcpy((uint8_t *)shmServer.addr, buffer_->data, buffer_->length);
#else
    VCamSource_FillFrame(handle, buffer_->data, buffer_->length, buffer_->width, buffer_->height);
#endif

#ifdef SAVEFILE
    fp.write(reinterpret_cast<char *>(buffer_->data), size);
#endif
    play_frame_count = 0;
    frame_count++;
}

void Cache::keepFpsThread() {
    while(running) {
        if (frame_count == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }
        if (play_frame_count > 2) {
            std::lock_guard lck(work_mtx_);
#ifdef __APPLE__
            memcpy((uint8_t *)shmServer.addr, buffer_->data, buffer_->length);
#else
            VCamSource_FillFrame(handle, buffer_->data, buffer_->length, buffer_->width, buffer_->height);
#endif

#ifdef SAVEFILE
            fp.write(reinterpret_cast<char *>(buffer_->data), buffer_->length);
#endif
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        play_frame_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    running = false;
}

void Cache::PlayQueue() {
    if (running) {
        return;
    }
    running = true;
    keepfps_ = std::thread(&Cache::keepFpsThread, this);
#ifdef __APPLE__
    shmServer.key = 5678;
    shmServer.shmid = shmget(shmServer.key, SHMSZ, 0666);
    if (shmServer.shmid < 0) {
        shmServer.shmid = shmget(shmServer.key, SHMSZ, IPC_CREAT | 0666);
    }
    if (shmServer.shmid < 0) {
        return;
    }
    shmServer.addr = shmat(shmServer.shmid, nullptr, 0);
#else
    VCamSource_Create(&handle);
    VCamSource_Init(handle);
#endif

#ifdef SAVEFILE
    fp.open("/Users/guochao/Downloads/air.rgb", std::ios::binary | std::ios::out);
#endif
}

void Cache::StopQueue() {
    play_frame_count = 0;
    if (!running) {
        return;
    }
    running = false;
    if (keepfps_.joinable()) {
        keepfps_.join();
    }
#ifdef __APPLE__
    shmdt(shmServer.addr);
    shmctl(shmServer.shmid, IPC_RMID, NULL);
#else
    VCamSource_Destroy(&handle);
    handle = nullptr;
#endif
    buffer_.reset();
    frame_count = 0;

#ifdef SAVEFILE
    fp.close();
#endif
}

void H264Decoder::Context::SetVideoSize(int w, int h) {
    default_screen_w = w;
    default_screen_h = h;
}

void H264Decoder::Context::Open() {
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        WriteErrorLog("avcodec_find_decoder error");
        return;
    }
    codecCtx_ = avcodec_alloc_context3(codec);
    if (!codecCtx_) {
        WriteErrorLog("avcodec_alloc_context3 error");
        return;
    }
    auto ret = avcodec_open2(codecCtx_, codec, nullptr);
    if (ret != 0) {
        WriteErrorLog("avcodec_open2 error");
        return;
    }
}

int H264Decoder::Context::Send(AVPacket *avpkt) {
    int ret = avcodec_send_packet(codecCtx_, avpkt);
    if (ret < 0) {
        WriteErrorLog("avcodec_send_packet error, %d", ret);
    }
    return ret;
}

int H264Decoder::Context::Receive() {
    if (src_frame) {
        av_frame_free(&src_frame);
        src_frame = nullptr;
    }
    if (filter_frame) {
        av_frame_free(&filter_frame);
        filter_frame = nullptr;
    }
    src_frame = av_frame_alloc();
    filter_frame = av_frame_alloc();

    if (!src_frame || !filter_frame) {
        WriteErrorLog("av_frame_alloc fail");
        return -1;
    }

    int ret = avcodec_receive_frame(codecCtx_, src_frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0) {
        av_frame_free(&src_frame);
        src_frame = nullptr;
        av_frame_free(&filter_frame);
        filter_frame = nullptr;
        if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            WriteErrorLog("avcodec_receive_frame error, %d", ret);
        }
    }
    return ret;
}

void H264Decoder::Context::Close() {
    if (sws_ctx) {
        sws_freeContext(sws_ctx);
        sws_ctx = nullptr;
    }
    if (src_frame) {
        av_frame_free(&src_frame);
        src_frame = nullptr;
    }
    if (filter_frame) {
        av_frame_free(&filter_frame);
        filter_frame = nullptr;
    }

    if (codecCtx_) {
        avcodec_close(codecCtx_);
        avcodec_free_context(&codecCtx_);
        codecCtx_ = nullptr;
    }
}

int H264Decoder::Context::Reset(uint8_t *data, int size) {
    if (codecCtx_) {
        avcodec_close(codecCtx_);
        avcodec_free_context(&codecCtx_);
        codecCtx_ = nullptr;
        codec = nullptr;
    }

    if (!codec) {
        codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    }
    if (!codecCtx_) {
        codecCtx_ = avcodec_alloc_context3(codec);
    }

    codecCtx_->extradata = new uint8_t(size);
    codecCtx_->extradata_size = size;
    memcpy(codecCtx_->extradata, data, size);
    codecCtx_->pix_fmt = AV_PIX_FMT_YUV420P;

    return avcodec_open2(codecCtx_, codec, nullptr);
}

void H264Decoder::Context::dealYuv(int& scalW, int& scalH) {
    if (src_frame->width < default_screen_w && src_frame->height < default_screen_h) {
    } else if (src_frame->width > default_screen_w && src_frame->height > default_screen_h) {
        if (src_frame->width * default_screen_h == src_frame->height * default_screen_w) {
            scalW = -1;
            scalH = -1;
        } else {
            int count = 1;
            int type = src_frame->width - default_screen_w > src_frame->height - default_screen_h ? 0 : 1;
            while (1) {
                int ret = type == 0 ? default_screen_w / count : default_screen_h / count;
                scalW = ret % 2 == 0 ? ret : ret + 1;
                auto line = type == 0 ? scalW * src_frame->height / src_frame->width : scalW * src_frame->width / src_frame->height;
                scalH = line % 2 == 0 ? line : line + 1;
                if (type == 0 && scalW <= default_screen_h) {
                    break;
                } else if (scalH <= default_screen_w) {
                    break;
                }
                count++;
            }

            if (type == 1) {
                std::swap(scalW, scalH);
            }
        }
    } else if (src_frame->width <= default_screen_w || src_frame->height <= default_screen_h) {
        if (src_frame->width < default_screen_w) {
            scalH = default_screen_h;
            auto lineW = scalH * src_frame->width / src_frame->height;
            scalW = lineW % 2 == 0 ? lineW : lineW + 1;
        } else {
            scalW = default_screen_w;
            auto lineH = scalW * src_frame->height / src_frame->width;
            scalH = lineH % 2 == 0 ? lineH : lineH + 1;
        }
    }
    return;
}

bool H264Decoder::Context::Update() {
    if (0 == video_size.width && 0 == video_size.height) {
        return true;
    }
    return video_size.width != src_frame->width || video_size.height != src_frame->height;
}

void H264Decoder::Close() {
    ctx.Close();
}

int H264Decoder::DecodeInternal(Context& ctx, uint8_t *buf, uint32_t size, bool isKey, uint32_t timestamp) {
    AVPacket packet;
    av_init_packet(&packet);
    packet.data = buf;
    packet.size = size;

    int ret = 0;
    ret = ctx.Send(&packet);
    if (ret < 0) {
        return ret;
    }

    while (ctx.Receive() >= 0) {
        if (ctx.Update()) {
            int scalW = 0;
            int scalH = 0;
            ctx.dealYuv(scalW, scalH);
            filter_.resetFilter();
            ret = filter_.initFilter(ctx.src_frame, scalW, scalH, ctx.default_screen_w, ctx.default_screen_h);
            if (ret < 0) {
                return ret;
            }
        }
        if (filter_.sendFrame(ctx.src_frame) < 0) {
            continue;
        }
        while (1) {
            ret = filter_.recvFrame(ctx.filter_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            ctx.video_size = {ctx.src_frame->width, ctx.src_frame->height};
            cache.Play(ctx.filter_frame, timestamp);
        }
    }
    return 0;
}

H264Decoder::H264Decoder() {
#if !defined(FF_API_NEXT)
    av_register_all();
    avcodec_register_all();
#endif
    av_log_set_level(AV_LOG_QUIET);
}

H264Decoder::~H264Decoder() {
    Close();
}

int H264Decoder::Decode(uint8_t *buf, uint32_t size, bool isKey, uint32_t timestamp) {
    return DecodeInternal(ctx, buf, size, isKey, timestamp);
}

int H264Decoder::resetContext(uint8_t *data, int size) {
    return ctx.Reset(data, size);
}

bool H264Decoder::Init(const std::string &stream_id) {
    return play_internal.Init(stream_id);
}

bool H264Decoder::Destroy() {
    return play_internal.Destroy();
}

void H264Decoder::Play() {
    ctx.Open();
    cache.PlayQueue();
}

void H264Decoder::Stop() {
    cache.StopQueue();
    ctx.Close();
}

void H264Decoder::SetWindowSize(int w, int h) {
    ctx.SetVideoSize(w, h);
}

}
