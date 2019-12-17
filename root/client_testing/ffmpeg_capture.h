#pragma once

#include <iostream>

#include "../src_code/codec/VideoRecorder.h"
#include "../src_code/codec/H264Encoder.h"
#include "../src_code/utils/Time.h"
#include "../src_code/stream/RtmpSender.h"
#include "../src_code/display/SDLPlayer.h"

bool playFrameData(const uint8_t *data, int width, int height, int linesize);

int ffmpeg_capture();