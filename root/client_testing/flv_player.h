#pragma once

//todo: 仅支持audio：speex/video：h264的flv数据，16000 1 s16
#include "../src_code/offline/FlvParse.h"
#include "../src_code/display/MediaState.h"

#include "Param.h"

int flv_player(Argument& cmd);