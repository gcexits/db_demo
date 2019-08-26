#include "H264Encoder.h"

namespace duobei {
namespace video {
void H264Encoder::Reset() {
    videoContext.resetContext();
}

bool H264Encoder::DesktopEncode(uint8_t* videoBuffer, int dstWidth, int dstHeight, int devType, uint32_t timestamp) {
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

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        return false;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(videoContext.codec.ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            break;
        }

        bool isKeyFrame = pkt->flags & AV_PKT_FLAG_KEY;
        rtmpObject.sendH264Packet(pkt->data, pkt->size, isKeyFrame, timestamp, false);
//        fp_out.write((char *)pkt->data, pkt->size);
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);
    return true;
}

H264Encoder::H264Encoder() : rtmpObject("rtmp://utx-live.duobeiyun.net/live/guochao") {
    fp_out.open("./test_encode.h264", std::ios::binary | std::ios::ate);
    av_log_set_level(AV_LOG_QUIET);
}
}  // namespace video
}  // namespace duobei
