#pragma once

extern "C" {
#define __STDC_CONSTANT_MACROS
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
}

#include <sstream>

namespace duobei::filter {

class Filter {
    AVFilterContext *buffersink_ctx = nullptr;
    AVFilterContext *buffersrc_ctx = nullptr;
    AVFilterGraph *filter_graph = nullptr;

    std::string getFilters(int scaleW, int scaleH, int dstW, int dstH);
public:
    explicit Filter() = default;
    virtual ~Filter() = default;

    int initFilter(int srcW, int srcH, int scaleW, int scaleH, int fmt, int dstW, int dstH, AVRational time_base, AVRational sample_aspect_ratio);

    int sendFrame(AVFrame *src_frame);

    int recvFrame(AVFrame *dst_frame);

    void resetFilter();
};

}