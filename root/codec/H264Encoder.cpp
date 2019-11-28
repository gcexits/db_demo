#include "H264Encoder.h"

namespace duobei {
namespace video {
void H264Encoder::Reset() {
    videoContext.resetContext();
    if (pkt) {
        av_packet_free(&pkt);
    }
}

bool H264Encoder::DesktopEncode(uint8_t* videoBuffer, int dstWidth, int dstHeight, int devType) {
    conversion.fillBuffer(videoBuffer, dstWidth, dstHeight, devType, dstWidth, dstHeight);
    if (conversion.resolutionChange()) {
        Reset();
    }

    conversion.convertFrame(devType);

    if (!videoContext.codec.Opened()) {
        videoContext.codec.SetCodec(dstWidth, dstHeight);
        if (!videoContext.codec.OpenCodec()) {
            return false;
        }
    }

    conversion.setCodecOptions(AV_PIX_FMT_YUV420P);

    auto ret = avcodec_send_frame(videoContext.codec.ctx, conversion.dst.Frame);
    if (ret < 0) {
        return false;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(videoContext.codec.ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return false;
        }
        if (ret == 0) {
            break;
        }
    }
    return true;
}

H264Encoder::H264Encoder() {
    av_log_set_level(AV_LOG_QUIET);
    pkt = av_packet_alloc();
}
}  // namespace video
}  // namespace duobei
