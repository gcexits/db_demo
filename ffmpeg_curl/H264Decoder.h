#pragma once

#include <mutex>
#include <fstream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}

#include "Optional.h"

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
        playInternal.Init("video");
    }

    bool OpenDecode(AVCodecContext* codecCtx_) {
        if (codecCtx_) {
            this->codecCtx_ = codecCtx_;
            videoCodec = avcodec_find_decoder(codecCtx_->codec_id);
        } else {
            videoCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
            codecCtx_ = avcodec_alloc_context3(videoCodec);
        }
        int ret = avcodec_open2(codecCtx_, videoCodec, nullptr);
        assert(ret == 0);
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

    bool Decode(uint8_t* buf, uint32_t size);
};