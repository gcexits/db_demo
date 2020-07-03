#include "Filter.h"

namespace duobei::filter {

std::string Filter::getFilters(int scaleW, int scaleH, int dstW, int dstH, int srcW, int srcH) {
    char parse[256] = {0};
    auto oss = std::stringstream();
    int rimW = 0;
    int rimH = 0;
    if (scaleW == 0 && scaleH == 0) {
        rimH = (dstH - srcH) / 2;
        rimW = (dstW - srcW) / 2;
        snprintf(parse, 256, "pad=%d:%d:%d:%d:black", dstW, dstH, rimW, rimH);
    } else if (scaleW == -1 && scaleH == -1) {
        snprintf(parse, 256, "scale=%d:%d", dstW, dstH);
    } else {
        if (scaleW == dstW) {
            rimH = (dstH - scaleH) / 2;
        } else {
            rimW = (dstW - scaleW) / 2;
        }
        // todo: 缩放，加黑框，垂直镜像
        snprintf(parse, 256, "scale=%d:%d,pad=%d:%d:%d:%d:black", scaleW, scaleH, dstW, dstH, rimW, rimH);
    }
    oss << parse;
#ifndef __APPLE__
    oss << ",vflip";
#endif
    return oss.str();
}

int Filter::setFilter(AVFrame *frame, AVFilterGraph *filter_graph, AVFilterInOut *output) {
    auto oss = std::stringstream();

    oss << "video_size=" << frame->width << "x" << frame->height
        << ":pix_fmt=" << frame->format
        << ":time_base=" << 1 << "/" << 100000
        << ":pixel_aspect=" << frame->width << "/" << frame->height;

    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    int ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                           oss.str().c_str(), nullptr, filter_graph);
    if (ret < 0) {
        return ret;
    }

    output->name = av_strdup("in");
    output->filter_ctx = buffersrc_ctx;
    output->pad_idx = 0;
    output->next = nullptr;

    return 0;
}

int Filter::initFilter(AVFrame *frame, int scaleW, int scaleH, int dstW, int dstH) {
    filter_graph = avfilter_graph_alloc();
    if (!filter_graph) {
        return -1;
    }
    AVFilterInOut *output = avfilter_inout_alloc();
    setFilter(frame, filter_graph, output);

    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    if (avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",nullptr, nullptr, filter_graph) < 0) {
        return -1;
    }

#ifdef __APPLE__
    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_RGB24, AV_PIX_FMT_NONE};
#else
    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_BGR24, AV_PIX_FMT_NONE};
#endif
    if (av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN) < 0) {
        return -1;
    }

    AVFilterInOut *input = avfilter_inout_alloc();
    input->name = av_strdup("out");
    input->filter_ctx = buffersink_ctx;
    input->pad_idx = 0;
    input->next = nullptr;

    std::string filters_descr = getFilters(scaleW, scaleH, dstW, dstH, frame->width, frame->height);
    if (avfilter_graph_parse_ptr(filter_graph, filters_descr.c_str(), &input, &output, nullptr) < 0) {
        return -1;
    }

    if (avfilter_graph_config(filter_graph, nullptr) < 0) {
        return -1;
    }

    avfilter_inout_free(&input);
    avfilter_inout_free(&output);
    return 0;
}

int Filter::sendFrame(AVFrame *src_frame) {
    return av_buffersrc_add_frame_flags(buffersrc_ctx, src_frame, AV_BUFFERSRC_FLAG_KEEP_REF);
}

int Filter::recvFrame(AVFrame *dst_frame) {
    return av_buffersink_get_frame(buffersink_ctx, dst_frame);
}

void Filter::resetFilter() {
    if (buffersink_ctx) {
        avfilter_free(buffersink_ctx);
        buffersink_ctx = nullptr;
    }

    if (buffersrc_ctx) {
        avfilter_free(buffersrc_ctx);
        buffersrc_ctx = nullptr;
    }

    if (filter_graph) {
        avfilter_graph_free(&filter_graph);
        filter_graph = nullptr;
    }
}

}