
#include "Media.h"
#include <iostream>

extern "C"{
#include <libavutil/time.h>
}
extern bool quit;

MediaState::MediaState(std::string input_file)
	:filename(input_file)
{
	pFormatCtx = nullptr;
	audio = new AudioState();

	video = new VideoState();
	//quit = false;
}

MediaState::~MediaState()
{
	if(audio)
		delete audio;

	if (video)
		delete video;
}

bool MediaState::openInput()
{
	// Open input file
	if (avformat_open_input(&pFormatCtx, filename.c_str(), nullptr, nullptr) < 0)
		return false;

	if (avformat_find_stream_info(pFormatCtx, nullptr) < 0)
		return false;

	// Output the stream info to standard 
	av_dump_format(pFormatCtx, 0, filename.c_str(), 0);

	audio->stream_index = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    video->stream_index = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);

	if (audio->stream_index < 0 || video->stream_index < 0)
		return false;

	// Fill audio state
	AVCodec *pCodec = avcodec_find_decoder(pFormatCtx->streams[audio->stream_index]->codecpar->codec_id);
	if (!pCodec)
		return false;

	audio->stream = pFormatCtx->streams[audio->stream_index];

	audio->audio_ctx = avcodec_alloc_context3(pCodec);
	if (avcodec_parameters_to_context(audio->audio_ctx, pFormatCtx->streams[audio->stream_index]->codecpar) != 0)
		return false;

	avcodec_open2(audio->audio_ctx, pCodec, nullptr);

	// Fill video state
	AVCodec *pVCodec = avcodec_find_decoder(pFormatCtx->streams[video->stream_index]->codecpar->codec_id);
	if (!pVCodec)
		return false;

	video->stream = pFormatCtx->streams[video->stream_index];

	video->video_ctx = avcodec_alloc_context3(pVCodec);
	if (avcodec_parameters_to_context(video->video_ctx, pFormatCtx->streams[video->stream_index]->codecpar) != 0)
		return false;

	avcodec_open2(video->video_ctx, pVCodec, nullptr);

	video->frame_timer = static_cast<double>(av_gettime()) / 1000000.0;
	video->frame_last_delay = 40e-3;

	return true;
}

int decode_thread(void *data)
{
	MediaState *media = (MediaState*)data;

	AVPacket *packet = av_packet_alloc();

	while (true)
	{
		int ret = av_read_frame(media->pFormatCtx, packet);
		if (ret < 0)
		{
			if (ret == AVERROR_EOF)
				break;
			if (media->pFormatCtx->pb->error == 0) // No error,wait for user input
			{
				SDL_Delay(100);
				continue;
			}
			else
				break;
		}

		if (packet->stream_index == media->audio->stream_index) // audio stream
		{
			media->audio->audioq.enQueue(packet);
			av_packet_unref(packet);
		}		

		else if (packet->stream_index == media->video->stream_index) // video stream
		{
			media->video->videoq->enQueue(packet);
			av_packet_unref(packet);
		}		
		else
			av_packet_unref(packet);
	}

	av_packet_free(&packet);

	return 0;
}