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

struct DecoderContextInterface {
    virtual bool Init() = 0;
    virtual void Reset() = 0;
    virtual int Decode(char* in, uint32_t len, uint32_t timestemp) = 0;
    virtual ~DecoderContextInterface() = default;

    std::function<void(void* data, int size, uint32_t timestamp)> output_fn_ = nullptr;
    int frame_size = 320;
    short* output_frame = nullptr;
    char type = 0;
};

struct OpusDecoderContext: DecoderContextInterface {
    OpusDecoder* dec = nullptr;
    size_t empty_frame_count = 0;

    bool Init() override;

    void Reset() override {
        if (dec) {
            opus_decoder_destroy(dec);
            dec = nullptr;
        }
        if (output_frame) {
            delete[] output_frame;
            output_frame = nullptr;
        }
    }

    int Decode(char* in, uint32_t size, uint32_t timestamp) override;
    int DecodeInternal(const unsigned char* data, opus_int32 len, uint32_t timestamp, int decode_fec);
};
}  // namespace audio
}  // namespace duobei
