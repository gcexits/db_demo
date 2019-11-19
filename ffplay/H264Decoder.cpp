#include "H264Decoder.h"
#include <cassert>

bool H264Decode::Decode(AVPacket *pkt, AVFrame *frame_) {
    int result = avcodec_send_packet(codecCtx_, pkt);
    if (result < 0) {
        return false;
    }
    av_packet_unref(pkt);

    do {
        result = avcodec_receive_frame(codecCtx_, frame_);
        if (result > 0) {
            frame_->pts = frame_->best_effort_timestamp;
        }
        if (result == AVERROR_EOF) {
            return false;
        }
        if (result >= 0) {
            break;
        }
    } while (result != AVERROR(EAGAIN));
    return true;
}
