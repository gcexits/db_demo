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

    std::string getFilters(int scaleW, int scaleH, int dstW, int dstH, int srcW, int srcH);
    int setFilter(AVFrame *frame, AVFilterGraph *filter_graph, AVFilterInOut *output);

public:
    explicit Filter() = default;
    virtual ~Filter() = default;

    int initFilter(AVFrame *frame, int scaleW, int scaleH, int dstW, int dstH);

    int sendFrame(AVFrame *src_frame);

    int recvFrame(AVFrame *dst_frame);

    void resetFilter();
};

}