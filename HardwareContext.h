//
// Created by Johnny on 7/10/2019.
//

#pragma once


#include <cassert>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avassert.h>
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}

class HardwareContext {
    enum AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
    enum AVPixelFormat pix_fmt = AV_PIX_FMT_NONE;
    AVBufferRef *device_ctx = nullptr;

    enum AVHWDeviceType find_hwdevice_type(const char *name) const {
        auto type = av_hwdevice_find_type_by_name(name);  // videotoolbox
        if (type != AV_HWDEVICE_TYPE_NONE) {
            return type;
        }

        av_log(nullptr, AV_LOG_INFO, "Device type %s is not supported.\n", name);
        av_log(nullptr, AV_LOG_INFO, "Available device types:");
        while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
            av_log(nullptr, AV_LOG_INFO, " %s", av_hwdevice_get_type_name(type));
        av_log(nullptr, AV_LOG_INFO, "\n");
        return type;
    }

    enum AVPixelFormat get_hw_pix_fmt(const AVCodec *decoder, enum AVHWDeviceType type) const {
        for (int i = 0;; i++) {
            const AVCodecHWConfig *config = avcodec_get_hw_config(decoder, i);
            if (!config) {
                av_log(nullptr, AV_LOG_INFO, "Decoder %s does not support device type %s.\n",
                       decoder->name, av_hwdevice_get_type_name(type));
                return AV_PIX_FMT_NONE;
            }
            if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
                config->device_type == type) {
                return config->pix_fmt;
            }
        }
    }


    static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
                                            const enum AVPixelFormat *pix_fmts) {
        auto hw_ctx = static_cast<HardwareContext *>(ctx->opaque);
        for (auto p = pix_fmts; *p != AV_PIX_FMT_NONE; ++p) {
            if (*p == hw_ctx->pix_fmt)
                return *p;
        }

        av_log(nullptr, AV_LOG_INFO, "Failed to get HW surface format.\n");
        return AV_PIX_FMT_NONE;
    }

public:
    void find(const char *name, const AVCodec *decoder) {
        type = find_hwdevice_type(name);
        assert(AV_HWDEVICE_TYPE_NONE != type);
        pix_fmt = get_hw_pix_fmt(decoder, type);
        assert(pix_fmt != AV_PIX_FMT_NONE);
    }

    const AVFrame *transfer(AVFrame *dst, const AVFrame *src) {
        if (static_cast<AVPixelFormat>(src->format) == pix_fmt) {
            /* retrieve data from GPU to CPU */
            if (av_hwframe_transfer_data(dst, src, 0) < 0) {
                av_log(nullptr, AV_LOG_INFO, "Error transferring the data to system memory\n");
                return nullptr;
            }
            return dst;
        } else {
            return src;
        }
    }

    int init(AVCodecContext *ctx) {
        ctx->opaque = this;
        if (type == AV_HWDEVICE_TYPE_NONE) {
            return 0;
        }
        int err = av_hwdevice_ctx_create(&device_ctx, type, nullptr, nullptr, 0);
        if (err < 0) {
            av_log(nullptr, AV_LOG_INFO, "Failed to create specified HW device.\n");
            return err;
        }
        ctx->hw_device_ctx = av_buffer_ref(device_ctx);
        ctx->get_format = get_hw_format;
        return err;
    }

    void reset() {
        type = AV_HWDEVICE_TYPE_NONE;
        pix_fmt = AV_PIX_FMT_NONE;
        if (device_ctx != nullptr) {
            av_buffer_unref(&device_ctx);
        }
    }

    const char *hwdevice_name() const {
        return av_hwdevice_get_type_name(type);
    }
};
