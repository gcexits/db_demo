#pragma once

#include <iostream>

#include <speex/speex.h>
#include <speex/speex_types.h>

#include "Optional.h"

class SpeexDecode {
    SpeexBits bits;
    void* dec_state = nullptr;
    int frame_size = 0;                 //每帧的大小
    short int* output_frame = nullptr;  //输出的大小
    audio::PlayInternal playInternal;
public:
    SpeexDecode() {
        speex_bits_init(&bits);
        dec_state = speex_decoder_init(speex_lib_get_mode(SPEEX_MODEID_WB));
        speex_decoder_ctl(dec_state, SPEEX_GET_FRAME_SIZE, &frame_size);

        int speex_enh = 1;
        speex_decoder_ctl(dec_state, SPEEX_SET_ENH, &speex_enh);
        output_frame = new short[frame_size];
        playInternal.Init();
    }

    ~SpeexDecode() {
        speex_bits_destroy(&bits);
        speex_decoder_destroy(dec_state);
        if (output_frame) {
            delete[] output_frame;
        }
        playInternal.Destroy();
    }

    int Decode(char *data, uint32_t size, uint8_t *pcm);
};