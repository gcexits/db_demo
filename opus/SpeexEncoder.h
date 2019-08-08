#pragma once


#include <functional>
#include "speex/speex.h"
#include "speex/speex_types.h"

#include "OpusEncoder.h"

namespace duobei {
namespace audio {
struct SpeexEncoderContext: EncoderContextInterface {
    enum { pcmChunk = 16000 / 25 };  // 640
    uint8_t pcmBuffer[pcmChunk] = {0};
    int pcmPostion = 0;
    bool Init() override;
    void Reset() override;
    void Encode(uint8_t *data, int size) override;
    void eatData(void* data, int size);

    const static int SAMPLE_RATE = 16000;
    const static int frame_size = SAMPLE_RATE / 50;
    const static int max_frame = 8;
    const static int samplerate = SAMPLE_RATE;  //采样率 16000 /
    int enc_frame_size = 0;

    mutable std::mutex mtx_;
    SpeexBits enc_bits;  //speex encoder
    void* enc_state = nullptr;

    int pkt_frame_count = 0;                 //当前帧数
    const static int frames_per_packet = 2;  //编码时每次帧数

    static const int audio_length_ =  1+frame_size * frames_per_packet;
    int8_t audio_buffer_[audio_length_];
    int8_t* speex_buffer_ = audio_buffer_ + 1;

    std::ofstream fp_write;
};

}  // namespace audio
}  // namespace duobei
