#include "SpeexDecoder.h"

int SpeexDecode::Decode(char *data, uint32_t size, uint8_t *pcm) {
    if (size == 0 || !data) {
        return 0;
    }
    int frame_count = 0;
    //将编码数据如读入bits
    speex_bits_reset(&bits);
    speex_bits_read_from(&bits, data, size);
    // 对帧进行解码
    int ret = speex_decode_int(dec_state, &bits, output_frame);
    while (ret == 0) {
#if !defined(USING_SDL)
        memcpy(pcm, output_frame, frame_size * 2);
#else
        playInternal.Play(output_frame, frame_size * 2);
#endif
        ret = speex_decode_int(dec_state, &bits, output_frame);
    }
    return frame_count;
}