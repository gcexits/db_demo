#include "Filter.h"

namespace duobei::filter {

std::string Filter::getFilters(int scaleW, int scaleH, int dstW, int dstH) {
    int rimW = 0;
    int rimH = 0;
    if (scaleW == dstW) {
        rimH = (dstH - scaleH) / 2;
    } else {
        rimW = (dstW - scaleW) / 2;
    }
    char parse[256] = {0};
    auto oss = std::stringstream();
    snprintf(parse, 256, "scale=%d:%d,pad=%d:%d:%d:%d:black", scaleW, scaleH, dstW, dstH, rimW, rimH);
    oss << parse;
    return oss.str();
}

int Filter::initFilter(int srcW, int srcH, int scaleW, int scaleH, int fmt, int dstW, int dstH, AVRational time_base, AVRational sample_aspect_ratio) {
    char args[512];
    int ret = 0;
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_BGR24, AV_PIX_FMT_NONE};

    filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !filter_graph) {
        return -1;
    }

    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             srcW, srcH, fmt, time_base.num, time_base.den,
             sample_aspect_ratio.num, sample_aspect_ratio.den);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, nullptr, filter_graph);
    if (ret < 0) {
        return -1;
    }

    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       nullptr, nullptr, filter_graph);
    if (ret < 0) {
        return -1;
    }

    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        return -1;
    }

    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    std::string filters_descr = getFilters(scaleW, scaleH, dstW, dstH);
    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr.c_str(), &inputs, &outputs, nullptr)) < 0)
        return -1;

    if ((ret = avfilter_graph_config(filter_graph, nullptr)) < 0)
        return -1;

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    return 0;
}

void Filter::resetFilter() {

}

int Filter::sendFrame(AVFrame *src_frame) {
    return av_buffersrc_add_frame_flags(buffersrc_ctx, src_frame, AV_BUFFERSRC_FLAG_KEEP_REF);
}

int Filter::recvFrame(AVFrame *dst_frame) {
    return av_buffersink_get_frame(buffersink_ctx, dst_frame);
}

}