
#ifndef MEDIA_H
#define MEDIA_H

#include <string>
#include "Audio.h"
#include "Video.h"

extern "C" {

#include <libavformat/avformat.h>

}

struct VideoState;

struct MediaState
{
	AudioState *audio;
	VideoState *video;
	AVFormatContext *pFormatCtx;

    std::string filename;
	//bool quit;

	MediaState(std::string filename);

	~MediaState();

	bool openInput();
};

int decode_thread(void *data);

#endif