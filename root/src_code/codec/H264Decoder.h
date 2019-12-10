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

struct VideoState_ {
    AVStream *stream;           // video stream

    double video_clock;

    double synchronize(AVFrame *srcFrame, double pts) {
        double frame_delay;

        if (pts != 0)
            video_clock = pts;  // Get pts,then set video clock to it
        else
            pts = video_clock;  // Don't get pts,set it to video clock

        frame_delay = av_q2d(stream->time_base);
        frame_delay += srcFrame->repeat_pict * (frame_delay * 0.5);

        video_clock += frame_delay;

        return pts;
    }

    VideoState_() {
        video_clock = 0.0;
        stream = nullptr;
    }

    ~VideoState_() = default;
};

class H264Decode {
public:
    video::PlayInternal playInternal;
    VideoState_ videoState;
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
    bool Decode(AVPacket *pkt, uint32_t size);
};
