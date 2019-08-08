#pragma once

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <functional>
#include "opus/opus.h"
#include "opus/opus_multistream.h"
#include "opus/opus_types.h"


namespace duobei {
namespace audio {

struct EncoderContextInterface {
    virtual bool Init() = 0;
    virtual void Reset() = 0;
    virtual void Encode(uint8_t *data, int size) = 0;
    virtual ~EncoderContextInterface() = default;

    std::function<void(const int8_t*, int)> output_fn_ = nullptr;
};

struct OpusEncoderContext: EncoderContextInterface {
    OpusEncoder *enc = nullptr;
    static const int frame_bytes = 640;
    int frame_size = 320;
    static const int audio_length_ = 1+frame_bytes;
    int8_t audio_buffer_[audio_length_];
    char *opusout = (char*)audio_buffer_ + 1;

    bool Init() override;

    void Reset() override {
        if (enc) {
            opus_encoder_destroy(enc);
            enc = nullptr;
        }
        fp_write.close();
    }

    void Encode(uint8_t *in, int len) override;

    std::ofstream fp_write;
};
}  // namespace audio
}  // namespace duobei
