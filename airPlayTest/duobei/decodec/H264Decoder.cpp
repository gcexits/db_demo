#include "H264Decoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

#include <iostream>

namespace duobei::video {

void H264Decoder::Context::Open() {
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        WriteErrorLog("avcodec_find_decoder error");
        return;
    }
    codecCtx_ = avcodec_alloc_context3(codec);
    if (!codecCtx_) {
        WriteErrorLog("avcodec_alloc_context3 error");
        return;
    }
    auto ret = avcodec_open2(codecCtx_, codec, nullptr);
    if (ret != 0) {
        WriteErrorLog("avcodec_open2 error");
        return;
    }
}

int H264Decoder::Context::Send(AVPacket *avpkt) {
    int ret = avcodec_send_packet(codecCtx_, avpkt);
    if (ret < 0) {
        WriteErrorLog("avcodec_send_packet error, %s", av_err2str(ret));
    }
    return ret;
}

int H264Decoder::Context::Receive() {
    if (src_frame) {
        av_frame_free(&src_frame);
        src_frame = nullptr;
    }
    if (filter_frame) {
        av_frame_free(&filter_frame);
        filter_frame = nullptr;
    }
    src_frame = av_frame_alloc();
    filter_frame = av_frame_alloc();

    if (!src_frame || !filter_frame) {
        WriteErrorLog("av_frame_alloc fail");
        return -1;
    }

    int ret = avcodec_receive_frame(codecCtx_, src_frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0) {
        av_frame_free(&src_frame);
        src_frame = nullptr;
        av_frame_free(&filter_frame);
        filter_frame = nullptr;
        if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            WriteErrorLog("avcodec_receive_frame error, %s", av_err2str(ret));
        }
    }
    return ret;
}

void H264Decoder::Context::Close() {
    if (sws_ctx) {
        sws_freeContext(sws_ctx);
        sws_ctx = nullptr;
    }
    if (src_frame) {
        av_frame_free(&src_frame);
        src_frame = nullptr;
    }
    if (filter_frame) {
        av_frame_free(&filter_frame);
        filter_frame = nullptr;
    }

    if (codecCtx_) {
        avcodec_close(codecCtx_);
        avcodec_free_context(&codecCtx_);
        codecCtx_ = nullptr;
    }
}

int H264Decoder::Context::Reset(uint8_t *data, int size) {
    if (codecCtx_) {
        avcodec_close(codecCtx_);
        avcodec_free_context(&codecCtx_);
        codecCtx_ = nullptr;
        codec = nullptr;
    }

    if (!codec) {
        codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    }
    if (!codecCtx_) {
        codecCtx_ = avcodec_alloc_context3(codec);
    }

    codecCtx_->extradata = new uint8_t(size);
    codecCtx_->extradata_size = size;
    memcpy(codecCtx_->extradata, data, size);
    codecCtx_->pix_fmt = AV_PIX_FMT_YUV420P;

    return avcodec_open2(codecCtx_, codec, nullptr);
}

bool H264Decoder::Context::dealYuv(int dstPixelFormat, int dstWidth, int dstHeight, int rimWidth, int rimHeight) {
    return true;
}

void H264Decoder::Close() {
    ctx.Close();
}

void H264Decoder::Play(AVFrame *frame, uint32_t timestamp) {
    int size = av_image_get_buffer_size((AVPixelFormat)frame->format, frame->width, frame->height, 1);
    auto *buf = new uint8_t[size];
    av_image_copy_to_buffer(buf, size, frame->data, frame->linesize, static_cast<AVPixelFormat>(frame->format), frame->width, frame->height, 1);
//    fp.write(reinterpret_cast<char *>(buf), size);
    fp.write((char *)frame->data[0], size);
    //WriteErrorLog("🤣🤣🤣 sucess Play packet !!!");
//    play_internal.Play((void *)frame->data[0], size, frame->width, frame->height, timestamp);
    delete[] buf;
}

int H264Decoder::DecodeInternal(Context& ctx, uint8_t *buf, uint32_t size, bool isKey, uint32_t timestamp) {
    AVPacket packet;
    av_init_packet(&packet);
    packet.data = buf;
    packet.size = size;

    int ret = 0;
    ret = ctx.Send(&packet);
    if (ret < 0) {
        return ret;
    }

    while (ctx.Receive() >= 0) {
        // 608x1080
        // todo: 1. w | h 小 2. w & h 小 3. w & h 大
        int scalW = 0;
        int scalH = 0;
        if (isKey) {
            if (ctx.src_frame->width < default_screen_w && ctx.src_frame->height < default_screen_h) {
            } else if (ctx.src_frame->width < default_screen_w && ctx.src_frame->height < default_screen_h) {
            } else if (ctx.src_frame->width < default_screen_w || ctx.src_frame->height < default_screen_h) {
                if (ctx.src_frame->width < default_screen_w) {
                    scalH = default_screen_h;
                    auto lineW = scalH * ctx.src_frame->width / ctx.src_frame->height;
                    scalW = lineW % 2 == 0 ? lineW : lineW + 1;
                } else {
                }
            }
            AVRational time_base = {1, 10000};
            AVRational sample_aspect_ratio = {76, 135};
            ret = filter_.initFilter(ctx.src_frame->width, ctx.src_frame->height, scalW, scalH, ctx.src_frame->format, default_screen_w, default_screen_h, time_base, sample_aspect_ratio);
        }

        if (filter_.sendFrame(ctx.src_frame) < 0) {
            continue;
        }
        while (1) {
            ret = filter_.recvFrame(ctx.filter_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            Play(ctx.filter_frame, timestamp);
        }
    }
    return 0;
}

H264Decoder::H264Decoder() {
#if !defined(FF_API_NEXT)
    av_register_all();
    avcodec_register_all();
#endif
    av_log_set_level(AV_LOG_QUIET);
    ctx.Open();
    fp.open("/Users/guochao/Downloads/test.rgb", std::ios::binary | std::ios::out);
}

H264Decoder::~H264Decoder() {
    Close();
}

int H264Decoder::Decode(uint8_t *buf, uint32_t size, bool isKey, uint32_t timestamp) {
    return DecodeInternal(ctx, buf, size, isKey, timestamp);
}

int H264Decoder::resetContext(uint8_t *data, int size) {
    return ctx.Reset(data, size);
}

bool H264Decoder::Init(const std::string &stream_id) {
    return play_internal.Init(stream_id);
}

bool H264Decoder::Destroy() {
    return play_internal.Destroy();
}

}
