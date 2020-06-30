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
#include <fstream>

#include "PlayInternal.h"
#include "Filter.h"

struct AVCodecContext;
struct AVCodec;
struct AVFrame;
struct SwsContext;
struct AVPacket;

namespace duobei::video {

class H264Decoder {
    struct Context {
        AVCodecContext *codecCtx_ = nullptr;
        AVCodec *codec = nullptr;
        SwsContext *sws_ctx = nullptr;

        AVFrame *src_frame = nullptr;
        AVFrame *filter_frame = nullptr;

        void Open();
        int Send(AVPacket *avpkt);
        int Receive();
        void Close();
        int Reset(uint8_t *data, int size);
        bool dealYuv(int dstPixelFormat, int dstWidth, int dstHeight, int rimWidth, int rimHeight);
    };
    Context ctx;
    filter::Filter filter_;

    void Close();
    void Play(AVFrame *frame, uint32_t timestamp);
    int DecodeInternal(Context &ctx, uint8_t *buf, uint32_t size, bool isKey, uint32_t timestamp);

    std::ofstream fp;
    int default_screen_w = 1280;
    int default_screen_h = 720;

public:
    PlayInternal play_internal;

    explicit H264Decoder();
    virtual ~H264Decoder();

    int Decode(uint8_t *buf, uint32_t size, bool isKey, uint32_t timestamp);
    int resetContext(uint8_t *data, int size);
    bool Init(const std::string &stream_id);
    bool Destroy();

};
}
