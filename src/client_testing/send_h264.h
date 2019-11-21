#pragma once

#include "../utils/Time.h"
#include "../codec/H264Encoder.h"
#include "../stream/RtmpSender.h"

#include <thread>
#include <fstream>

int send_h264();