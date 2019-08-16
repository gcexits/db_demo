#include "H264Decoder.h"
#include <cassert>

std::fstream video_fp;

bool H264Decode::Decode(uint8_t *buf, uint32_t size) {
#if defined(SRCFILE)
    if (!video_fp.is_open()) {
        video_fp.open("123.yuv", std::ios::binary | std::ios::ate | std::ios::out);
    }
#endif
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
        uint8_t *yuv = new uint8_t[frame->width * frame->height * 1.5];
        int size = av_image_get_buffer_size((AVPixelFormat)frame->format, frame->width, frame->height, 1);
        assert(size > 0);
        int ret = av_image_copy_to_buffer(yuv, size, (const uint8_t *const *)frame->data, (const int *)frame->linesize, (AVPixelFormat)frame->format, frame->width, frame->height, 1);
        assert(ret > 0);
#if !defined(USING_SDL)
        width = frame->width;
        height = frame->height;
        assert(ret > 0);
#else
        playInternal.Play(yuv, size, frame->width, frame->height);
#endif

#if defined(SRCFILE)
        video_fp.write((char *)yuv, frame->width * frame->height * 1.5);
#endif
        delete[] yuv;
    }
    return true;
}
