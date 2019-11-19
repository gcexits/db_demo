
#include "Video.h"
#include "VideoDisplay.h"

extern "C" {

#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

VideoState::VideoState() {
    stream = nullptr;

    frame = av_frame_alloc();

    videoq = new PacketQueue();

    frame_timer = 0.0;
    frame_last_delay = 0.0;
    frame_last_pts = 0.0;
    video_clock = 0.0;

    running = false;
}

VideoState::~VideoState() {
    delete videoq;

    running = false;

    av_frame_free(&frame);
}

void VideoState::video_play(MediaState *media) {
    running = true;
    decoder_tid = SDL_CreateThread(decode, "", this);

    schedule_refresh(media, 40);  // start display
}

double VideoState::synchronize(AVFrame *srcFrame, double pts) {
    double frame_delay;

    if (pts != 0)
        video_clock = pts;  // Get pts,then set video clock to it
    else
        pts = video_clock;  // Don't get pts,set it to video clock

    frame_delay = av_q2d(stream->time_base);
    frame_delay += srcFrame->repeat_pict * (frame_delay * 0.5);

    video_clock += frame_delay;

    return pts;
}

int decode(void *arg) {
    VideoState *video = (VideoState *)arg;

    AVFrame *frame = av_frame_alloc();

    AVPacket packet;
    double pts;

    while (video->running) {
        video->videoq->deQueue(&packet, true);

        if (video->h264Decode.Decode(&packet, frame)) {

            pts = av_q2d(video->stream->time_base) * frame->pts;

            pts = video->synchronize(frame, pts);

            frame->opaque = &pts;

            if (video->frameq.nb_frames >= FrameQueue::capacity)
                SDL_Delay(500 * 2);

            video->frameq.enQueue(frame);

            av_frame_unref(frame);
        }
    }

    av_frame_free(&frame);

    return 0;
}
