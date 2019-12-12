#pragma once

#include <mutex>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include "libavutil/time.h"
}

#include "../utils/Optional.h"


class H264Decode {
public:
    video::PlayInternal playInternal;
    double pts = 0.0;

    struct Context {
        struct VideoSize {
            int width = 0;
            int height = 0;
        };
        VideoSize video_size;
        AVCodecContext *codecCtx_ = nullptr;
        AVCodec *codec = nullptr;
        SwsContext *sws_ctx = nullptr;
        const AVCodecParameters *parameters = nullptr;

        AVFrame *src_frame = nullptr;
        AVFrame *dst_frame = nullptr;
        AVFrame *scale_frame = nullptr;
#if VIDEO_DECODING_USE_HARDWARE
        HardwareContext *hw_ctx = nullptr;
#endif

        bool Open(bool with_hw, const AVCodecParameters* param);
        int Send(const AVPacket *avpkt);

        int Receive();

        void Close();

        bool Update() const;
        bool Scaling(int dstPixelFormat);
    };
    Context context;
public:
    H264Decode() {
        av_log_set_level(AV_LOG_QUIET);
        playInternal.Init("video");
    }

    bool OpenDecode(const AVCodecParameters* param) {
        return context.Open(false, param);
    }

    ~H264Decode() {
        context.Close();
        playInternal.Destroy();
    }

    bool Decode(uint8_t* buf, uint32_t size);
    bool Decode(AVPacket *pkt, struct VideoState_1 *m);
};
