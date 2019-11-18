
#include "Media.h"
#include <iostream>

extern "C"{
#include <libavutil/time.h>
}

MediaState::MediaState()
{
    audio = new AudioState();
	video = new VideoState();
}

MediaState::~MediaState()
{
    if(audio)
        delete audio;

	if (video)
		delete video;
}