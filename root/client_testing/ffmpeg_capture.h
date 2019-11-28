#pragma once

#include <iostream>

#include "../src_code/codec/VideoRecorder.h"
#include "../src_code/codec/H264Encoder.h"
#include "../src_code/utils/Time.h"
#include "../src_code/stream/RtmpSender.h"

int ffmpeg_capture();