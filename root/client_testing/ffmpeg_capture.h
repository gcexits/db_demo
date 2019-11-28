#pragma once

#include <iostream>

#include "../codec/VideoRecorder.h"
#include "../codec/H264Encoder.h"
#include "../utils/Time.h"
#include "../stream/RtmpSender.h"

int ffmpeg_capture();