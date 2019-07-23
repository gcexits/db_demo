#include "SDLPlayer.h"

SDLPlayer* SDLPlayer::player = nullptr;

SDLPlayer::SDLPlayer() {
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO| SDL_INIT_EVENTS);

    audioSpec.freq = 16000;
    audioSpec.format = AUDIO_S16SYS;
    audioSpec.channels = 1;
    audioSpec.silence = 0;
    audioSpec.samples = 320;
    audioSpec.callback = AudioCallback;
    audioSpec.userdata = this;   //可以直接在内部传值给callback函数
    SDL_OpenAudio(&audioSpec,NULL);
    SDL_PauseAudio(0);

    SDL_CreateThread(RefreshLoop, "RefreshLoop", this);
}

SDLPlayer::~SDLPlayer() {
    SDL_CloseAudio();
    running = false;
    SDL_Quit();
}

SDLPlayer* SDLPlayer::getPlayer() {
    if(player == nullptr){
        player = new SDLPlayer;
    }
    return player;
}
#define ISSHE_REFRESH_EVENT 			(SDL_USEREVENT + 1)
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

void SDLPlayer::EventLoop() {
    SDL_Event event;
    while (running) {
        SDL_WaitEvent(&event);
        switch (event.type) {
            case SDL_QUIT:
            {
                SDL_Quit();
                break;
            }
            case ISSHE_REFRESH_EVENT:
            {
                videoContainer.show();
                break;
            }
            case SDL_KEYDOWN:
            {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                } else if (event.key.keysym.sym == SDLK_LEFT) {
                } else if (event.key.keysym.sym == SDLK_RIGHT) {
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            {
                break;
            }
            default:
            {
                break;
            }
        }

    }
    printf("Exit %s \n", __FUNCTION__);
}