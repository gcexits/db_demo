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
#include <queue>

#include "PlayInternal.h"
#include "Filter.h"
#include "util/Time.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <mach/mach_time.h>
#else
#include "libVCamMicSource.h"
#endif

namespace duobei::video {

#define SHMSZ 4194304

#ifdef __APPLE__
struct ShmServer {
    int shmid;
    key_t key;
    void *addr;
    int sequenceNumber;
};
#endif

struct Buffer {
    uint8_t *data = nullptr;
    size_t width = 0;
    size_t height = 0;
    size_t length = 0;
    explicit Buffer(AVFrame *data_, size_t length_) {
        if (length == 0) {
            data = new uint8_t[length_];
        } else {
            WriteDebugLog("length to big, remalloc");
            if (update(length_)) {
                delete []data;
                data = new uint8_t[length_];
            }
        }
        av_image_copy_to_buffer(data, length_, data_->data, data_->linesize, static_cast<AVPixelFormat>(data_->format), data_->width, data_->height, 1);
        width = data_->width;
        height = data_->height;
        length = length_;
    }

    bool update(int length_) {
        return length_ > length;
    }

    ~Buffer() {
        delete[] data;
    }

    using Ptr = std::unique_ptr<Buffer>;
};

struct Cache {
    Buffer::Ptr buffer_;
    std::mutex work_mtx_;
    int play_frame_count = 0;
    int frame_count = 0;
#ifdef __APPLE__
    ShmServer shmServer;
#else
    void *handle = nullptr;
#endif

    void Play(AVFrame *frame, uint32_t timestamp);

    bool running = false;
    std::thread keepfps_;
    void keepFpsThread();

    void PlayQueue();
    void StopQueue();

#ifdef SAVEFILE
    std::ofstream fp;
#endif
};

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
        AVFrame *filter_frame = nullptr;

        int default_screen_w = 480;
        int default_screen_h = 360;

        void SetVideoSize(int w, int h);
        void Open();
        int Send(AVPacket *avpkt);
        int Receive();
        void Close();
        int Reset(uint8_t *data, int size);
        void dealYuv(int& scalW, int& scalH);
        bool Update();
    };
    Context ctx;
    filter::Filter filter_;

    void Close();
    int DecodeInternal(Context &ctx, uint8_t *buf, uint32_t size, bool isKey, uint32_t timestamp);

    Cache cache;

public:
    PlayInternal play_internal;

    explicit H264Decoder();
    virtual ~H264Decoder();

    int Decode(uint8_t *buf, uint32_t size, bool isKey, uint32_t timestamp);
    int resetContext(uint8_t *data, int size);
    bool Init(const std::string &stream_id);
    bool Destroy();
    void Play();
    void Stop();
    void SetWindowSize(int w, int h);

};
}
