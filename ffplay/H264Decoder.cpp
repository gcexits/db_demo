#include "H264Decoder.h"
#include <cassert>

bool H264Decode::Decode(AVPacket *pkt, AVFrame *frame_) {
    int result = avcodec_send_packet(codecCtx_, pkt);
    if (result < 0) {
        return false;
    }

    result = avcodec_receive_frame(codecCtx_, frame_);
    if (result < 0 && result != AVERROR_EOF) {
        return false;
    }
    return true;
}
