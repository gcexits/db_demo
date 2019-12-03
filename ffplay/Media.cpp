
#include "Media.h"
#include <iostream>

extern "C"{
#include <libavutil/time.h>
}

MediaState::MediaState()
{
    audio = new AudioState();
}

MediaState::~MediaState()
{
    if(audio)
        delete audio;
}