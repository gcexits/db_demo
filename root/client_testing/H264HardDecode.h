#pragma once

#include <stdio.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avassert.h>
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}

#include "../src_code/codec/HardwareContext.h"
#include "../src_code/utils/Yuvscale.h"

#include <fstream>

int decode_write();

int h264HardDecode();