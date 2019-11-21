#pragma once

//https://ffmpeg.org/doxygen/trunk/encode__video_8c_source.html

#include "VideoBuffer.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
}

#include <fstream>

namespace duobei {
namespace video {
class H264Encoder {
    Video::VideoContext videoContext;

public:
    void Reset();

    H264Encoder();

    ~H264Encoder() {
        Reset();
    }

    bool DesktopEncode(uint8_t *videoBuffer, int dstWidth, int dstHeight, int devType, uint32_t timestamp);

    Video::VideoConversion conversion;
};

}  // namespace video
}  // namespace duobei
