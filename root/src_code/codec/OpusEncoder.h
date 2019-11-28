#pragma once

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <functional>
#include "opus/opus.h"
#include "opus/opus_multistream.h"
#include "opus/opus_types.h"


namespace duobei {
namespace audio {

struct EncoderContextInterface {
    virtual bool Init() = 0;
    virtual void Reset() = 0;
    virtual void Encode(uint8_t *data, int size, uint32_t ts) = 0;
    virtual ~EncoderContextInterface() = default;

    std::function<void(const int8_t*, int, uint32_t)> output_fn_ = nullptr;
};

struct OpusEncoderContext: EncoderContextInterface {
    OpusEncoder *enc = nullptr;
    static const int frame_bytes = 640;
    int frame_size = 320;
    static const int audio_length_ = 1+frame_bytes;
    int8_t audio_buffer_[audio_length_];
    char *opusout = reinterpret_cast<char*>(audio_buffer_) + 1;

    bool Init() override;

    void Reset() override {
        if (enc) {
            opus_encoder_destroy(enc);
            enc = nullptr;
        }
    }

    void Encode(uint8_t *in, int len, uint32_t ts) override;
};
}  // namespace audio
}  // namespace duobei
