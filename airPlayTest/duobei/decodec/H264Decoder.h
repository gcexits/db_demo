#pragma once

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <functional>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include "PlayInternal.h"

struct AVCodecContext;
struct AVCodec;
struct AVFrame;
struct SwsContext;
struct AVPacket;

namespace duobei::video {

class H264Decoder {
    struct Context {
        struct VideoSize {
            int width = 0;
            int height = 0;
        };
        VideoSize video_size;
        AVCodecContext *codecCtx_ = nullptr;
        AVCodec *codec = nullptr;
        SwsContext *sws_ctx = nullptr;

        AVFrame *src_frame = nullptr;
        AVFrame *dst_frame = nullptr;
        AVFrame *scale_frame = nullptr;

        void Open();
        int Send(AVPacket *avpkt);
        int Receive();
        void Close();
        int Reset(uint8_t *data, int size);
        bool Update() const;
        bool Scaling(int dstPixelFormat);
    };
    Context ctx;

    void Close();
    void Play(AVFrame *frame, uint32_t timestamp);
    int DecodeInternal(Context &ctx, uint8_t *buf, uint32_t size, uint32_t timestamp);

public:
    PlayInternal play_internal;

    explicit H264Decoder();
    virtual ~H264Decoder();

    int Decode(uint8_t *buf, uint32_t size, uint32_t timestamp);
    int resetContext(uint8_t *data, int size);
    bool Init(const std::string &stream_id);
    bool Destroy();

};
}
