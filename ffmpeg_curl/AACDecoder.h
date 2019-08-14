#pragma once

#include <fstream>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include "libswresample/swresample.h"
}

#include "Optional.h"

class AACDecode {
    AVCodecContext* codecCtx_ = nullptr;
    AVCodec* audioCodec = nullptr;
    AVFrame* frame = nullptr;
    audio::PlayInternal playInternal;
    SwrContext* pcm_convert = nullptr;
    bool inited = false;

    int channels = 0;
    int sampleRate = 0;
    int sampleFmt = -1;
    bool isInited() const {
        return (channels == 0 || sampleRate == 0 || sampleFmt == -1) ? false : true;
    }

public:
    AACDecode() {
        frame = av_frame_alloc();
        avcodec_register_all();
        av_log_set_level(AV_LOG_QUIET);
        playInternal.Init("audio");
    }

    bool OpenDecode(const AVCodecParameters* param) {
        if (!inited) {
            if (param) {
                codecCtx_ = avcodec_alloc_context3(nullptr);
                avcodec_parameters_to_context(codecCtx_, param);
                audioCodec = avcodec_find_decoder(codecCtx_->codec_id);
            } else {
                audioCodec = avcodec_find_decoder(AV_CODEC_ID_AAC);
                codecCtx_ = avcodec_alloc_context3(audioCodec);
            }
            int ret = avcodec_open2(codecCtx_, audioCodec, nullptr);
            assert(ret == 0);
            inited = true;
        }
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
    bool Decode(AVPacket *pkt);
};