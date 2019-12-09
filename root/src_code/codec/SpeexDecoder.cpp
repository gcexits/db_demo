#include "SpeexDecoder.h"

int SpeexDecode::Decode(char *data, uint32_t size) {
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
        playInternal.Play(output_frame, frame_size * 2, 0.0);
        ret = speex_decode_int(dec_state, &bits, output_frame);
    }
    return frame_count;
}