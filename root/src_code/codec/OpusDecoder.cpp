#include "OpusDecoder.h"
#include <cassert>

namespace duobei {
namespace audio {


bool OpusDecoderContext::Init() {
    type = 0xC6;
    opus_int32 sampling_rate = 16000;
    if (sampling_rate != 8000 && sampling_rate != 12000 &&
        sampling_rate != 16000 && sampling_rate != 24000 &&
        sampling_rate != 48000) {
        fprintf(stderr,
                "Supported sampling rates are 8000, 12000, "
                "16000, 24000 and 48000.\n");
        return false;
    }
    int frame_bytes = (sampling_rate * 2 * 20) / 1000;  // 1 hz 2B, 1000/50
    frame_size = frame_bytes / 2;
    output_frame = new short[frame_size];
    if (dec) {
        opus_decoder_destroy(dec);
    }
    int err = 0;
    dec = opus_decoder_create(sampling_rate, 1, &err);
    if (err != OPUS_OK) {
        fprintf(stderr, "cannnot decode opus: %s\n", opus_strerror(err));
        dec = NULL;
        return false;
    }
    return true;
}

int OpusDecoderContext::Decode(char *in, uint32_t size, uint32_t timestamp) {
    if (size == 0) {
        ++empty_frame_count;
        return 0;
    }
    auto data = reinterpret_cast<const unsigned char *>(in);
    auto len = static_cast<opus_int32>(size);
    if (empty_frame_count > 0) {
        for (size_t i = empty_frame_count; i>1; --i) { // 4 3 2
            DecodeInternal(data, 0, timestamp - i*frame_size/16, 0);
        }
        DecodeInternal(data, len, timestamp - frame_size/16, 1);  // 1
        empty_frame_count = 0;
    }
    return DecodeInternal(data, len, timestamp, 0);
}

int OpusDecoderContext::DecodeInternal(const unsigned char* data, opus_int32 len, uint32_t timestamp, int decode_fec) {
    assert(output_fn_);
    auto ret = opus_decode(dec, data, len, output_frame, frame_size, decode_fec);
    if (ret != frame_size) {
        fprintf(stderr, "opus_decode: %s\n", opus_strerror(ret));
        return -1;
    }
    output_fn_(output_frame, ret * 2, timestamp);
    return ret * 2;
}

}  // namespace audio
}  // namespace duobei
