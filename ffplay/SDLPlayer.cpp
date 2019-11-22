#pragma once

#include "SDLPlayer.h"

SDLPlayer* SDLPlayer::player = nullptr;

SDLPlayer::SDLPlayer() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);
}

SDLPlayer* SDLPlayer::getPlayer() {
    if (player == nullptr) {
        player = new SDLPlayer;
    }
    return player;
}

// note: space：暂停 ->：快进10s <-：快退10s q/esc：退出 1/2/3：倍速播放
void SDLPlayer::EventLoop() {
    SDL_Event event;
    while (running) {
        SDL_WaitEvent(&event);
        switch (event.type) {
            case SDL_QUIT:
                do_exit();
                break;
            case FF_REFRESH_EVENT:
                videoContainer.show();
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