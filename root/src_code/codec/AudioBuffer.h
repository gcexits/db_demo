#pragma once

#include <cassert>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "../hlring/RingBuffer.h"
#include "../utils/Time.h"

struct AVFormatContext;
struct AVCodecParameters;
struct AVCodecContext;
struct AVCodec;
struct SwrContext;
struct AVFrame;
struct AVPacket;
struct AVIOContext;

namespace duobei {

namespace audio {

struct AudioOption {
    explicit AudioOption(int c, int sr, int sf)
        : channels(c), sampleRate(sr), sampleFmt(sf) {}
    ~AudioOption() = default;
    AudioOption& operator=(const AudioOption& rhs) = default;
    int channels = 0;
    int sampleRate = 0;
    int sampleFmt = -1;
    bool inited() {
        return channels != 0 && sampleRate != 0 && sampleFmt != -1;
    }
    bool changed(int ch, int sr, int sf) {
        if (ch != channels || sr != sampleRate || sf != sampleFmt) {
            return true;
        }
        return false;
    }
    int nb_samples() const;
};
class CodecContext;

class AudioData {
    std::vector<uint8_t> buffer;

    void allocFrame();
    void samplingChange(int c, int sRate, int sFmt) {
        if (channels != c || sampleRate != sRate || sampleFmt != sFmt) {
            channels = c;
            sampleRate = sRate;
            sampleFmt = sFmt;
        }
    }

public:
    int bufferSize = 0;

    int channels = 0;
    int sampleRate = 0;
    int sampleFmt = -1;

    AVFrame* Frame = nullptr;

    bool isInited() const {
        return bufferSize == 0 || channels == 0 || sampleRate == 0 || !Frame || sampleFmt == -1 ? false : true;
    }

    bool isChanged(int ch, int sr, int sf) {
        return ch != channels || sr != sampleRate || sf != sampleFmt;
    }

    bool FormatChanged(int sample_fmt) {
        return sampleFmt != sample_fmt;
    }

    void setCodecOptions(int nb_samples);
    void updateFrame(const AudioOption& o);
    void updateFrame(const CodecContext& o);
    void updateFrame(const AVCodecContext* c);
    void fillFrame();
    void fillFrame(const uint8_t* src, int size);
    void resetFrame();
    int BufferSize();

    explicit AudioData() {
        allocFrame();
    }
    ~AudioData() {
        resetFrame();
    }
};

class CodecContext {
public:
    bool initCodec = false;

    // 音频编码参数
    int channels = 1;
    int sampleRate = 16000;
    int sampleFmt = 1;

public:
    CodecContext() = default;
    ~CodecContext() {
        Close();
    }
    // 音频编解码器上下文
    AVCodecContext* decodeCtx = nullptr;
    AVCodec* decoder = nullptr;

    // 音频编解码器
    bool Opened() {
        return initCodec;
    }
    void Close();
    bool SetCodec(const AVCodecParameters* param);
    bool OpenCodec();
};

struct AudioSampling {
    AudioSampling() = default;
    struct SwrContext* pcm_convert = nullptr;
    void reset();
    void resetFrameContext();
    AudioData src;
    AudioData dst;

    bool DataInit() const {
        return src.isInited() && dst.isInited();
    }

    void fillBuffer();
    void setDstFrameOptions();
    bool convert();
    ~AudioSampling() {
        reset();
    }
};

struct AudioContext {
    struct AudioSampling sampling;
    CodecContext codec;
    void setSrcFrameOptions();
    void setDstFrameOptions();

    AudioContext() = default;

    ~AudioContext() {
        resetContext();
    }

    void resetContext() {
        codec.Close();
    }
};
}  // namespace audio
}  // namespace duobei
