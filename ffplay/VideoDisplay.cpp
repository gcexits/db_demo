#include "VideoDisplay.h"
#include <iostream>

// 延迟delay ms后刷新video帧
void schedule_refresh(int delay) {
    SDL_AddTimer(delay, sdl_refresh_timer_cb, nullptr);
}

uint32_t sdl_refresh_timer_cb(uint32_t interval, void *opaque) {
    SDL_Event event;
    event.type = FF_REFRESH_EVENT;
    SDL_PushEvent(&event);
    return 0;
}
