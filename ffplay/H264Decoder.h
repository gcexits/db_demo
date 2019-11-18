#pragma once

#include <fstream>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class H264Decode {
public:
    AVCodecContext* codecCtx_ = nullptr;
    AVCodec* videoCodec = nullptr;
    SwsContext *sws_ctx = nullptr;
    bool inited = false;

    H264Decode() {
        av_log_set_level(AV_LOG_QUIET);
    }

    bool OpenDecode(const AVCodecParameters* param) {
        if (!inited) {
            if (param) {
                codecCtx_ = avcodec_alloc_context3(nullptr);
                assert(avcodec_parameters_to_context(codecCtx_, param) == 0);
                videoCodec = avcodec_find_decoder(codecCtx_->codec_id);
            } else {
                videoCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
                codecCtx_ = avcodec_alloc_context3(videoCodec);
            }
            int ret = avcodec_open2(codecCtx_, videoCodec, nullptr);
            assert(ret == 0);
            inited = true;
        }
        return true;
    }

    ~H264Decode() {
        if (codecCtx_) {
            avcodec_close(codecCtx_);
            avcodec_free_context(&codecCtx_);
            codecCtx_ = nullptr;
        }
        if (sws_ctx) {
            sws_freeContext(sws_ctx);
            sws_ctx = nullptr;
        }
        inited = false;
    }

    bool Decode(AVPacket* pkt, AVFrame *scale_frame);
};