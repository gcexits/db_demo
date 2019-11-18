
#include "VideoDisplay.h"
#include <iostream>

// 延迟delay ms后刷新video帧
void schedule_refresh(MediaState *media, int delay) {
    SDL_AddTimer(delay, sdl_refresh_timer_cb, media);
}

uint32_t sdl_refresh_timer_cb(uint32_t interval, void *opaque) {
    SDL_Event event;
    event.type = FF_REFRESH_EVENT;
    event.user.data1 = opaque;
    SDL_PushEvent(&event);
    return 0; /* 0 means stop timer */
}
