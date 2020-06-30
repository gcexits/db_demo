#pragma once

#include <iostream>
#include <thread>
#include <fstream>

#include "../src_code/codec/VideoRecorder.h"
#include "../src_code/codec/H264Encoder.h"
#include "../src_code/codec/SpeexEncoder.h"

#include "../src_code/demux/Demuxer.h"

#include "../src_code/display/SDLPlayer.h"
#include "../src_code/display/MediaState.h"

#include "../src_code/hlring/RingBuffer.h"
#include "../src_code/hlring/rbuf.h"

#include "../src_code/network/HttpFile.h"

#include "../src_code/offline/FlvParse.h"

#include "../src_code/stream/RtmpSender.h"

#include "../src_code/utils/Time.h"
#include "../src_code/utils/Optional.h"

#include "../src_code/common/PacketParser.h"

#include "Param.h"

int ffplay(Argument& cmd);

/****************************************/

int flv_player(Argument& cmd);

/****************************************/

bool playFrameData(uint8_t *data, int width, int height, int linesize);

int ffmpeg_capture(Argument& cmd);

/****************************************/

int send_h264(Argument& cmd);

/****************************************/

int send_speex(Argument& cmd);

/****************************************/

int rtmp_recv(Argument& cmd);