#include "H264Decoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswresample/swresample.h>
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
    if (dst_frame) {
        av_frame_free(&dst_frame);
        dst_frame = nullptr;
    }
    src_frame = av_frame_alloc();
    dst_frame = av_frame_alloc();

    if (!src_frame || !dst_frame) {
        return -1;
    }

    int ret = avcodec_receive_frame(codecCtx_, src_frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0) {
        av_frame_free(&src_frame);
        src_frame = nullptr;
        av_frame_free(&dst_frame);
        dst_frame = nullptr;
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
    if (dst_frame) {
        av_frame_free(&dst_frame);
        dst_frame = nullptr;
    }
    if (scale_frame) {
        av_frame_free(&scale_frame);
        scale_frame = nullptr;
    }

    if (codecCtx_) {
        avcodec_close(codecCtx_);
        avcodec_free_context(&codecCtx_);
        codecCtx_ = nullptr;
    }
}

int H264Decoder::Context::Reset(uint8_t *data, int size) {
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

bool H264Decoder::Context::Update() const {
    if (0 == video_size.width || 0 == video_size.height) {
        return false;
    }
    if (0 == codecCtx_->width || 0 == codecCtx_->height) {
        return true;
    }
    if (codecCtx_->width != video_size.width || codecCtx_->height != video_size.height) {
        return true;
    }
    return false;
}

bool H264Decoder::Context::Scaling(int pix_fmt) {
    src_frame->width = codecCtx_->width;
    src_frame->height = codecCtx_->height;
    auto f = src_frame;

    if (!sws_ctx) {
        sws_ctx = sws_getContext(f->width, f->height, static_cast<AVPixelFormat>(f->format),
                                 f->width, f->height, static_cast<AVPixelFormat>(pix_fmt),
                                 SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!sws_ctx) {
            return false;
        }
    }

    if (scale_frame) {
        av_frame_free(&scale_frame);
        scale_frame = nullptr;
    }
    scale_frame = av_frame_alloc();
    if (!scale_frame) {
        return false;
    }

    scale_frame->format = pix_fmt;
    scale_frame->width = f->width;
    scale_frame->height = f->height;
    auto len = av_image_alloc(scale_frame->data, scale_frame->linesize, scale_frame->width, scale_frame->height,
                              static_cast<AVPixelFormat>(scale_frame->format), 1);

    if (len < 0) {
        return false;
    }

    auto ret = sws_scale(sws_ctx, f->data, f->linesize, 0, scale_frame->height, scale_frame->data,
                         scale_frame->linesize);

    auto success = ret > 0;
    if (success && scale_frame->height > 0 && scale_frame->width > 0) {
        video_size.height = scale_frame->height;
        video_size.width = scale_frame->width;
    }
    return success;
}

void H264Decoder::Close() {
    ctx.Close();
}

void H264Decoder::Play(AVFrame *frame, uint32_t timestamp) {
    int size = av_image_get_buffer_size((AVPixelFormat)frame->format, frame->width, frame->height, 1);
    play_internal.Play((void *)frame->data[0], size, frame->width, frame->height, timestamp);
}

int H264Decoder::DecodeInternal(Context& ctx, uint8_t *buf, uint32_t size, uint32_t timestamp) {
    AVPacket packet;
    av_init_packet(&packet);
    packet.data = buf;
    packet.size = size;

    auto ret = ctx.Send(&packet);
    if (ret < 0) {
        return ret;
    }

    if (ctx.Update()) {
        ctx.Close();
        ctx.Open();
        ret = ctx.Send(&packet);
        if (ret < 0) {
            return ret;
        }
    }

    while (ctx.Receive() >= 0) {
        auto success = ctx.Scaling(0);
        if (success) {
            Play(ctx.scale_frame, timestamp);
            av_freep(&ctx.scale_frame->data[0]);
        } else {

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
}

H264Decoder::~H264Decoder() {
    Close();
}

int H264Decoder::Decode(uint8_t *buf, uint32_t size, uint32_t timestamp) {
    return DecodeInternal(ctx, buf, size, timestamp);
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
