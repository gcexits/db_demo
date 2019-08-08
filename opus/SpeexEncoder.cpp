#include "SpeexEncoder.h"

namespace duobei {
namespace audio {
bool SpeexEncoderContext::Init() {
    audio_buffer_[0] = 0xB2;
    int quality = 6;
    int sample_rate, vad = 1, denoise = 1, noiseSuppress = -21;

    speex_bits_init(&enc_bits);
    enc_state = speex_encoder_init(&speex_wb_mode);
    speex_encoder_ctl(enc_state, SPEEX_SET_QUALITY, &quality);
    int tmp = 2;
    speex_encoder_ctl(enc_state, SPEEX_SET_COMPLEXITY, &tmp);

    speex_encoder_ctl(enc_state, SPEEX_GET_FRAME_SIZE, &enc_frame_size);
    speex_encoder_ctl(enc_state, SPEEX_GET_SAMPLING_RATE, &sample_rate);
    pkt_frame_count = 0;
    return true;
}

// note: 仅被析构使用
void SpeexEncoderContext::Reset() {
    std::lock_guard<std::mutex> lck(mtx_);
    if (enc_state) {
        speex_encoder_destroy(enc_state);
        enc_state = nullptr;
    }
    speex_bits_destroy(&enc_bits);
    fp_write.close();
}

void SpeexEncoderContext::Encode(uint8_t *data, int size) {
    if (size > frame_size * 2) {
        return;
    }
    speex_encode_int(enc_state, (short *)data, &enc_bits);

    pkt_frame_count++;
    if (pkt_frame_count == frames_per_packet) {
        memset(speex_buffer_, 0, frame_size * frames_per_packet);
        int enc_size = speex_bits_write(&enc_bits, (char *)speex_buffer_, frame_size * frames_per_packet);
        speex_bits_reset(&enc_bits);
#if 0
        assert(output_fn_);
        if (frame_size * frames_per_packet > enc_size) {
            output_fn_(audio_buffer_, 1+enc_size);
        }
#else
        if (!fp_write.is_open()) {
            fp_write.open("./test.speex", std::ios::out | std::ios::binary | std::ios::ate);
        }
        if (frame_size * frames_per_packet > enc_size) {
            fp_write.write((char *)audio_buffer_, enc_size + 1);
        }
#endif
        pkt_frame_count = 0;
    }
}

void SpeexEncoderContext::eatData(void *data, int size) {
    int pcmNeeded = pcmChunk - pcmPostion;
    if (size >= pcmNeeded) {
        int dataPostion = size;
        if (pcmPostion > 0) {
            memcpy(pcmBuffer + pcmPostion, data, pcmNeeded);
            dataPostion -= pcmNeeded;
            Encode(pcmBuffer, pcmChunk);
            pcmPostion = 0;
        }
        while (dataPostion >= pcmChunk) {
            Encode((uint8_t *)data + size - dataPostion, pcmChunk);
            dataPostion -= pcmChunk;
        }
        if (dataPostion > 0) {
            pcmPostion = dataPostion;
            memcpy(pcmBuffer, (char *)data + size - dataPostion, pcmPostion);
        }
    } else {
        memcpy(pcmBuffer + pcmPostion, data, size);
        pcmPostion += size;
    }
}
}  // namespace audio
}  // namespace duobei
