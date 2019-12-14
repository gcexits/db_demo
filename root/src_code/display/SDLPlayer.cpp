#include "SDLPlayer.h"

SDLPlayer::SDLPlayer() {
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);
}

// note: space：暂停 ->：快进10s <-：快退10s q/esc：退出 1/2/3：倍速播放
void SDLPlayer::EventLoop(struct MediaState& m) {
    SDL_Event event;
    while (running) {
        SDL_WaitEvent(&event);
        switch (event.type) {
            case FF_REFRESH_EVENT:
                videoContainer.show(m);
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_SPACE:
                        break;
                    case SDLK_LEFT:
                        break;
                    case SDLK_RIGHT:
                        break;
                    case SDLK_q:
                    case SDLK_ESCAPE:
                        m.exit();
                        do_exit();
                        break;
                    default:
                        break;
                }
            case SDL_MOUSEBUTTONDOWN:
                break;
            case SDLK_SPACE:
            default:
                break;
        }
    }
    printf("Exit %s \n", __FUNCTION__);
}
