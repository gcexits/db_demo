#pragma once

#include <fstream>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "Optional.h"

class AACDecode {
    AVCodecContext* codecCtx_ = nullptr;
    AVCodec* audioCodec = nullptr;
    AVFrame* frame = nullptr;
    audio::PlayInternal playInternal;

public:
    AACDecode() {
        frame = av_frame_alloc();
        avcodec_register_all();
        av_log_set_level(AV_LOG_QUIET);
        playInternal.Init("audio");
    }

    bool OpenDecode(AVCodecContext* codecCtx_) {
        if (codecCtx_) {
            this->codecCtx_ = codecCtx_;
            audioCodec = avcodec_find_decoder(codecCtx_->codec_id);
        } else {
            audioCodec = avcodec_find_decoder(AV_CODEC_ID_AAC);
            codecCtx_ = avcodec_alloc_context3(audioCodec);
        }
        int ret = avcodec_open2(codecCtx_, audioCodec, nullptr);
        assert(ret == 0);
    }

    ~AACDecode() {
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