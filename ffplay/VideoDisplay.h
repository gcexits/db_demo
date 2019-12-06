
#ifndef VIDEO_DISPLAY_H
#define VIDEO_DISPLAY_H
extern "C" {
#include <libavutil/time.h>
#include <libswscale/swscale.h>
}

#include <SDL2/SDL.h>

#define FF_REFRESH_EVENT (SDL_USEREVENT)
#define FF_QUIT_EVENT (SDL_USEREVENT + 1)

static const double SYNC_THRESHOLD = 0.01;
static const double NOSYNC_THRESHOLD = 10.0;

void schedule_refresh(int delay);

uint32_t sdl_refresh_timer_cb(uint32_t interval, void *opaque);

//void video_display(VideoState *video);


#endif