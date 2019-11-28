#pragma once


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

static const char *filter_descr = "scale=78:24,transpose=cclock";
static AVFormatContext *fmt_ctx;
static AVCodecContext *dec_ctx;
static AVFilterContext *buffersink_ctx;
static AVFilterContext *buffersrc_ctx;
static AVFilterGraph *filter_graph;
static int video_stream_index = -1;

int open_input_file(const char *filename);

int init_filters(const char *filters_descr);

int simplest_ffmpeg_video_filter();