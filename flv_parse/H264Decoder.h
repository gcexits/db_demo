#pragma once

#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}

#include "Optional.h"
#include "libyuv.h"

class H264Decode {
    AVCodecContext* codecCtx_ = nullptr;
    AVCodec* videoCodec = nullptr;
    AVFrame *frame = nullptr;
    video::PlayInternal playInternal;
public:
    bool success = false;
    H264Decode() {
        frame = av_frame_alloc();
        avcodec_register_all();
        av_log_set_level(AV_LOG_QUIET);
        videoCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
        codecCtx_ = avcodec_alloc_context3(videoCodec);
        avcodec_open2(codecCtx_, videoCodec, nullptr);
        playInternal.Init("video");
    }

    ~H264Decode() {
        if (frame) {
            av_frame_free(&frame);
        }
        if (codecCtx_) {
            avcodec_close(codecCtx_);
            avcodec_free_context(&codecCtx_);
        }
        playInternal.Destroy();
    }

    bool Decode(uint8_t* buf, uint32_t size, int& width, int& height, uint8_t* yuv);
};