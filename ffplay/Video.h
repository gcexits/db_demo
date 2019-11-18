
#ifndef VIDEO_H
#define VIDEO_H

#include "PacketQueue.h"
#include "FrameQueue.h"
#include "H264Decoder.h"
#include "Media.h"

extern "C" {

#include <libavformat/avformat.h>

}

struct MediaState;
/**
 * 播放音频所需的数据封装
 */
struct VideoState
{
    H264Decode h264Decode;
	PacketQueue* videoq;        // 保存的video packet的队列缓存
    AVStream *stream;           // video stream

	FrameQueue frameq;          // 保存解码后的原始帧数据
	AVFrame *frame = nullptr;
    SDL_Thread *decoder_tid;
    bool running = false;

	double frame_timer;         // Sync fields
	double frame_last_pts;
	double frame_last_delay;
	double video_clock;

	void video_play(MediaState *media);

	double synchronize(AVFrame *srcFrame, double pts);
	
	VideoState();

	~VideoState();
};


int decode(void *arg); // 将packet解码，并将解码后的Frame放入FrameQueue队列中


#endif