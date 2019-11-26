#pragma once

#include <functional>

#include "AudioBuffer.h"
#include "OpusEncoder.h"
#include "speex/speex.h"
#include "speex/speex_types.h"

#define SAMPLE_RATE 16000

namespace duobei {
namespace audio {

struct AudioConfigure {
    std::string uid;
    audio::AudioOption src{1, 16000, 1};  // Record 输入， Play 输出
    audio::AudioOption dst{1, 16000, 1};  // Record 输出， Play 输入 16k

    bool sampling() const {
        return src.sampleRate != dst.sampleRate;
    }
};

struct SpeexEncoderContext : EncoderContextInterface {
    bool Init() override;
    void Reset() override;
    void Encode(uint8_t* data, int size) override;

    const static int frame_size = SAMPLE_RATE / 50;
    const static int max_frame = 8;
    const static int samplerate = SAMPLE_RATE;  //采样率 16000 /
    int enc_frame_size = 0;

    mutable std::mutex mtx_;
    SpeexBits enc_bits;  //speex encoder
    void* enc_state = nullptr;

    int pkt_frame_count = 0;                 //当前帧数
    const static int frames_per_packet = 2;  //编码时每次帧数

    static const int audio_length_ = 1 + frame_size * frames_per_packet;
    int8_t audio_buffer_[audio_length_];
    int8_t* speex_buffer_ = audio_buffer_ + 1;
};

class AudioEncoder {
    struct PCM {
        // Chunking property
        enum { kChunk = SAMPLE_RATE / 25 };  // 640
        uint8_t buffer[kChunk] = {0};
        int postion = 0;
    } pcm;

public:
    std::shared_ptr<EncoderContextInterface> encoder_;

    AudioEncoder(bool use_opus_);
    ~AudioEncoder() {
        encoder_->Reset();
    }
    AudioConfigure audio;

    void Chunking(void* data, int size);

    audio::AudioSampling sampling;

    void Sampling(void* data, int size);

    void Encode(void* data, size_t size);
};

class AudioSenderInterface {
protected:
    AudioEncoder* audio_encoder_ = nullptr;

public:
    virtual void SendAudioBuffer(const int8_t* data, int size) = 0;
    AudioSenderInterface() = default;
    virtual ~AudioSenderInterface() = default;

    void setAudioEncoder(AudioEncoder& encoder) {
        audio_encoder_ = &encoder;
        using namespace std::placeholders;
        audio_encoder_->encoder_->output_fn_ = std::bind(&AudioSenderInterface::SendAudioBuffer, this, _1, _2);
    }
};
}  // namespace audio
}  // namespace duobei
