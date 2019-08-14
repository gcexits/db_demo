#pragma once

#include <fstream>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}

#include "Optional.h"

class H264Decode {
    AVCodecContext* codecCtx_ = nullptr;
    AVCodec* videoCodec = nullptr;
    AVFrame* frame = nullptr;
    video::PlayInternal playInternal;
    bool inited = false;

public:
    H264Decode() {
        frame = av_frame_alloc();
        avcodec_register_all();
        av_log_set_level(AV_LOG_QUIET);
        playInternal.Init("video");
    }

    bool OpenDecode(const AVCodecParameters* param) {
        if (!inited) {
            if (param) {
                codecCtx_ = avcodec_alloc_context3(nullptr);
                avcodec_parameters_to_context(codecCtx_, param);
                videoCodec = avcodec_find_decoder(codecCtx_->codec_id);
            } else {
                videoCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
                codecCtx_ = avcodec_alloc_context3(videoCodec);
            }
            int ret = avcodec_open2(codecCtx_, videoCodec, nullptr);
            assert(ret == 0);
            inited = true;
        }
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