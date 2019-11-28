#pragma once

extern "C" {
#include <libavformat/avformat.h>
}

#include "../codec/H264Decoder.h"
#include "../codec/AACDecoder.h"
#include "../display/SDLPlayer.h"

#include <thread>

class IOBufferContext {
public:
    int buffer_size = 1024 * 4;
    AVIOContext* avio_ctx = nullptr;
    AVFormatContext* fmt_ctx = nullptr;
    bool opened_ = false;

    RingBuffer ringBuffer;
    const size_t RingLength;
    std::mutex ringBufferMtx;

    int read_packet(uint8_t* buffer, int length) {
        int count = 100;
        while (ringBuffer.size() < length && !io_sync.exit) {
            if (--count == 0) {
                break;
            }
//            std::unique_lock<std::mutex> lk(io_sync.m);
//            io_sync.cv.wait(lk);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        if (io_sync.exit) {
            return AVERROR_EOF;
        }
        std::lock_guard<std::mutex> lck(ringBufferMtx);
        return ringBuffer.read(buffer, length);
    }

    struct {
        mutable std::mutex m;
        mutable std::condition_variable cv;
        //bool ready = false;
        bool exit = false;
    } io_sync;

    int OpenInput() {
        size_t max_length_ = 1024 * 320;
        size_t size = std::min(RingLength, max_length_);
        while (ringBuffer.size() < size / 2 && !io_sync.exit) {
            std::unique_lock<std::mutex> lk(io_sync.m);
            io_sync.cv.wait(lk);
        }

        if (io_sync.exit) {
            // IOBufferContext 退出
            return AVERROR_EXIT;
        }

        uint8_t* buffer = (uint8_t*)av_malloc(buffer_size);
        if (!buffer) {
            return AVERROR(ENOMEM);
        }
        avio_ctx = avio_alloc_context(buffer, buffer_size, 0, this, &IOBufferContext::ReadPacket, nullptr, nullptr);
        if (!avio_ctx) {
            return AVERROR(ENOMEM);
        }

        fmt_ctx = avformat_alloc_context();
        if (!fmt_ctx) {
            return AVERROR(ENOMEM);
        }
        fmt_ctx->pb = avio_ctx;
        fmt_ctx->flags = AVFMT_FLAG_CUSTOM_IO;
        AVDictionary *opts = nullptr;
        av_dict_set(&opts, "timeout", "6000", 0);

        int ret = avformat_open_input(&fmt_ctx, nullptr, nullptr, &opts);
        opened_ = ret == 0;
        return ret;
    }
    explicit IOBufferContext(int length) : ringBuffer("IOBufferContext", length), RingLength(length) {}
    ~IOBufferContext() {
        io_sync.exit = true;
        {
            std::unique_lock<std::mutex> lk(io_sync.m);
            io_sync.cv.notify_all();
        }

        if (avio_ctx) {
            av_freep(&avio_ctx->buffer);  // avio_ctx_buffer
            av_freep(&avio_ctx);
        }

        if (fmt_ctx) {
            avformat_close_input(&fmt_ctx);
            fmt_ctx = nullptr;
        }
    }

    int FillBuffer(uint8_t* buffer, int length) {
        int len = 0;
        if (length != 0 && buffer) {
            std::lock_guard<std::mutex> lck(ringBufferMtx);
            len = ringBuffer.write(buffer, length);
        }
        std::unique_lock<std::mutex> lk(io_sync.m);
        io_sync.cv.notify_all();
        return len;
    }

    static int ReadPacket(void* opaque, uint8_t* buffer, int length) {
        IOBufferContext* playback = (IOBufferContext*)opaque;
        return playback->read_packet(buffer, length);
    }

    void* getFormatContext(bool* status) {
        if (status) {
            *status = opened_;
        }
        return fmt_ctx;
    }
};

class Demuxer {
public:
    enum class ReadStatus {
        Video,
        Audio,
        Subtitle,
        Error,
        EndOff
    };

private:
    bool opened_ = false;
    int videoindex = -1;
    int audioindex = -1;
    int scan_all_pmts_set = 0;

public:
    Demuxer() {
        pkt = av_packet_alloc();
    }
    ~Demuxer() {
        Close();
        av_packet_free(&pkt);
    }
    bool exit = false;
    struct AVFormatContext* ifmt_ctx = nullptr;
    bool need_free_ = true;
    struct AVPacket* pkt = nullptr;
    H264Decode video_decode;
    AudioDecode audio_decode;
    bool Opened() {
        return opened_;
    }
    void Close() {
        if (Opened()) {
            if (need_free_ && ifmt_ctx) {
                avformat_close_input(&ifmt_ctx);
                avformat_free_context(ifmt_ctx);
                ifmt_ctx = nullptr;
            }
            scan_all_pmts_set = 0;
            videoindex = audioindex = -1;
            opened_ = false;
        }
    }

    bool Open(const std::string& inUrl) {
        return true;
    }
    bool Open(void* param);

    // todo: 添加sps pps头
    bool addSpsPps(AVPacket* pkt, AVCodecParameters* codecpar, std::string name);

    // todo: 视频:0, 音频:1, 字幕:2, 失败:-1
    ReadStatus ReadFrame();

    bool isKeyFrame() {
        return pkt->flags & AV_PKT_FLAG_KEY;
    }

    AVCodecParameters* CodecPar(int index) {
        return ifmt_ctx->streams[index]->codecpar;
    }
};