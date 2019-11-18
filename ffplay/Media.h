#ifndef MEDIA_H
#define MEDIA_H

#include <string>
#include "Video.h"
#include "Audio.h"

extern "C" {

#include <libavformat/avformat.h>

}

struct VideoState;

struct MediaState
{
    AudioState *audio;
	VideoState *video;

	MediaState();

	~MediaState();
};

#endif