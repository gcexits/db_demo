#pragma once

#include "SDLPlayer.h"

SDLPlayer* SDLPlayer::player = nullptr;

SDLPlayer::SDLPlayer() {
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);

    audioSpec.freq = 48000;
    audioSpec.format = AUDIO_S16SYS;
    audioSpec.channels = 2;
    audioSpec.silence = 0;
    audioSpec.samples = 1024;
    audioSpec.callback = AudioCallback;
    audioSpec.userdata = this;  //可以直接在内部传值给callback函数
    SDL_OpenAudio(&audioSpec, NULL);
    SDL_PauseAudio(0);
    SDL_CreateThread(RefreshLoop, "RefreshLoop", this);
}

SDLPlayer* SDLPlayer::getPlayer() {
    if (player == nullptr) {
        player = new SDLPlayer;
    }
    return player;
}
#define ISSHE_REFRESH_EVENT (SDL_USEREVENT + 1)
int SDLPlayer::RefreshLoop(void* arg) {
    SDLPlayer* that = (SDLPlayer*)arg;
    SDL_Event event;

    while (that->running) {
        event.type = ISSHE_REFRESH_EVENT;
        SDL_PushEvent(&event);
        SDL_Delay(40);
    }
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
    SDL_Delay(30);
    printf("Exit %s \n", __FUNCTION__);
    return 0;
}

// note: space：暂停 ->：快进10s <-：快退10s q/esc：退出 1/2/3：倍速播放
void SDLPlayer::EventLoop() {
    SDL_Event event;
    // todo: 清空事件队列，在RegisterPlayer时就已经开始发命令了
    while (SDL_PollEvent(&event) != 0)
        ;
    while (running) {
        SDL_WaitEvent(&event);
        switch (event.type) {
            case SDL_QUIT:
                do_exit();
                break;
            case ISSHE_REFRESH_EVENT:
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
            default:
                break;
        }
    }
    printf("Exit %s \n", __FUNCTION__);
}
