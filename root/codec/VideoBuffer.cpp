//
// Created by Johnny on 2/8/18.
//

#include "VideoBuffer.h"
#include <string.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace duobei {

enum AVPixelFormat getPixelFormat(int devType) {
    enum AVPixelFormat formats[] = {
        AV_PIX_FMT_BGR0,    // 0
        AV_PIX_FMT_NV21,    // 1
        AV_PIX_FMT_RGBA,    // 2
        AV_PIX_FMT_YUV420P  // 3
    };
    size_t formatIndex = static_cast<size_t>(devType);
    if (devType < 0 || formatIndex >= sizeof(formats) / sizeof(*formats)) {
        formatIndex = 0;
    }
    return formats[formatIndex];
}


namespace Video {

bool isNAL(const uint8_t *buf, int &len) {
    static const uint8_t nal3[] = {0x00, 0x00, 0x01};
    if (memcmp(buf, nal3, sizeof(nal3)) == 0) {
        len = 3;
        return true;
    }

    static const uint8_t nal4[] = {0x00, 0x00, 0x00, 0x01};
    if (memcmp(buf, nal4, sizeof(nal4)) == 0) {
        len = 4;
        return true;
    }

    return false;
}

bool isSPS(const uint8_t *buf, int &len) {
    // !!! warning (buf[i] & 0x1f) == 0x07
    //if (buf[i++] == 0x00 && buf[i++] == 0x00 && buf[i++] == 0x00 && buf[i++] == 0x01 && (buf[i] & 0x1f) == 0x07) //sps
    //https://cardinalpeak.com/blog/the-h-264-sequence-parameter-set/
    //const uint8_t sps[] = { 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a, 0xf8, 0x41, 0xa2 };
    if (!isNAL(buf, len)) {
        return false;
    }
    return (buf[len] & 0x1f) == 0x07;
}

bool isPPS(const uint8_t *buf, int &len) {
    //const uint8_t pps[] = { 0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x38, 0x80 };
    if (!isNAL(buf, len)) {
        return false;
    }
    return (buf[len] & 0x1f) == 0x08;
}
// is I frame
bool isIFM(const uint8_t *buf, int &len) {
    if (!isNAL(buf, len)) {
        return false;
    }
    return (buf[len] & 0x1f) == 0x05;
}

void VideoConversion::resetFrameContext() {
    src.resetFrame();
    dst.resetFrame();
}

bool VideoData::updateFrame(int width, int height) {
    if (resolutionChange(width, height)) {
        resetFrame();
        allocFrame();
        fillFrame(AV_PIX_FMT_YUV420P);
        return true;
    }
    return false;
}

void VideoData::allocFrame() {
    if (!Frame) {
        Frame = av_frame_alloc();
    }
}

void VideoData::fillFrame(const uint8_t *videoBuffer, int pixelFormat) {
    enum AVPixelFormat srcFormat = static_cast<AVPixelFormat>(pixelFormat);
    av_image_fill_arrays(Frame->data, Frame->linesize,
                         videoBuffer, srcFormat, Width, Height, 1);
}

void VideoData::fillFrame(int pixelFormat) {
    enum AVPixelFormat srcFormat = static_cast<AVPixelFormat>(pixelFormat);
    auto length = av_image_get_buffer_size(srcFormat, Width, Height, 1);
    if (buffer.capacity() < length) {
        buffer.resize(length, '\0');
    }
    fillFrame(buffer.data(), pixelFormat);
}

void VideoData::resetFrame() {
    if (Frame) {
        av_frame_free(&Frame);
        Frame = nullptr;
    }
}

void VideoData::setCodecOptions(const AVCodecContext *ctx) {
    Frame->format = ctx->pix_fmt;
    Frame->width = ctx->width;
    Frame->height = ctx->height;
    Width = ctx->width;
    Height = ctx->height;
    Frame->pts = ptsCount++;
}

void VideoData::setFrameOptions(int format) {
    Frame->format = (AVPixelFormat)format;
    Frame->width = Width;
    Frame->height = Height;
    Frame->pts = ptsCount++;
}

void VideoConversion::fillBuffer(const uint8_t *videoBuffer, int srcWidth, int srcHeight,
                                 int devType, int dstWidth, int dstHeight) {
    src.updateFrame(srcWidth, srcHeight);
    pixelType = devType;
    enum AVPixelFormat srcFormat = getPixelFormat(pixelType);
    src.fillFrame(videoBuffer, srcFormat);

    dst.updateFrame(dstWidth, dstHeight);
    dst.fillFrame(AV_PIX_FMT_YUV420P);
}

void VideoConversion::resetSwsContext() {
    if (img_convert) {
        sws_freeContext(img_convert);
        img_convert = nullptr;
    }
}

bool VideoConversion::convertFrame(int devType) {
    if (!img_convert) {
        enum AVPixelFormat srcFormat = getPixelFormat(devType);
        img_convert = sws_getContext(
            src.Width, src.Height, srcFormat,
            dst.Width, dst.Height, AV_PIX_FMT_YUV420P,
            SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
        if (!img_convert) {
            return false;
        }
    }
    sws_scale(img_convert, src.Frame->data, src.Frame->linesize, 0,
              src.Height, dst.Frame->data, dst.Frame->linesize);

    return true;
}

bool CodecContext::SetCodec(int width, int height) {
    if (!codec) {
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    }
    if (!codec) {
        return false;
    }
    ctx = avcodec_alloc_context3(codec);
    if (!ctx) {
        return false;
    }
    ctx->codec_id = AV_CODEC_ID_H264;
    ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    ctx->width = width;
    ctx->height = height;
    ctx->gop_size = 30;
    ctx->time_base = AVRational{1, 24};
    ctx->max_b_frames = 0;
    ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    ctx->thread_count = 0;  // 自动根据机器CPU核数来选择
    ctx->thread_type = 2;
    ctx->flags2 |= AV_CODEC_FLAG2_FAST;

    const int KB = 1000 * 8;
    ctx->rc_min_rate = 80 * KB;
    ctx->rc_max_rate = 120 * KB;
    // ctx->rc_buffer_size = 60 * KB;
    ctx->bit_rate = 180 * KB;

    return true;
}

bool CodecContext::OpenCodec() {
    AVDictionary *options = nullptr;
    // 目前为High的Profile，实时性效果可以保证
    av_dict_set(&options, "profile", "baseline", 0);
    //        av_dict_set(&options, "level", "3.0", 0);
    av_dict_set(&options, "preset", "ultrafast", 0);

    av_dict_set(&options, "tune", "zerolatency", 0);
    // 该参数能提画面高质量
    av_dict_set_int(&options, "trellis", 2, 0);
    av_dict_set(&options, "crf", "32", 0);
    // 该参数开启cabac, 参考帧数量为1, 开启去块化效果,开启块分析, 8x8分析
    av_dict_set(&options, "x264-params", "cabac=1:ref=1:deblock=1,1:analyse=p8x8,i8x8:8x8dct=1", 0);
    //        av_dict_set(&options, "x264-params", "ref=1:deblock=1,1:analyse=p8x8", 0);
    int ret = avcodec_open2(ctx, codec, &options);
    if (ret < 0) {
        avcodec_free_context(&ctx);
        ctx = nullptr;
        return false;
    }
    initCodec = true;
    return true;
}

void CodecContext::Close() {
    if (Opened()) {
        if (ctx) {
            avcodec_close(ctx);
            avcodec_free_context(&ctx);
            ctx = nullptr;
        }
        initCodec = false;
    }
}

int CodecContext::PacketSize() {
    return 3 * ctx->width * ctx->height;
}

}  // namespace Video
}  // namespace duobei
