#ifndef MEDIA_H
#define MEDIA_H

#include <string>
#include "Audio.h"

extern "C" {

#include <libavformat/avformat.h>

}

struct VideoState;

struct MediaState
{
    AudioState *audio;

	MediaState();

	~MediaState();
};

#endif