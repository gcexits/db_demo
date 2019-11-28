#pragma once

#include "../src_code/utils/Time.h"
#include "../src_code/codec/H264Encoder.h"
#include "../src_code/stream/RtmpSender.h"

#include <thread>
#include <fstream>

int send_h264();