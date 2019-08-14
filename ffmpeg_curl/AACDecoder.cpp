#include "AACDecoder.h"
#include <cassert>

std::fstream audio_fp;

bool AACDecode::Decode(uint8_t *buf, uint32_t size) {
    if (!audio_fp.is_open()) {
        audio_fp.open("123.pcm", std::ios::binary | std::ios::ate | std::ios::out);
    }
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = buf;
    pkt.size = size;

    int result = avcodec_send_packet(codecCtx_, &pkt);
    if (result < 0) {
        return false;
    }
    while (result == 0) {
        result = avcodec_receive_frame(codecCtx_, frame);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
            break;
        }
        if (result < 0) {
            return false;
        }
        int size = av_samples_get_buffer_size(nullptr, frame->channels, codecCtx_->frame_size, (AVSampleFormat)frame->format, 1);
        //        av_fast_malloc
    }
    return true;
}