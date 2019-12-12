#include "ffplay.h"

static uint32_t sdl_refresh_timer_cb(uint32_t interval, void *opaque) {
    SDL_Event event;
    event.type = FF_REFRESH_EVENT;
    SDL_PushEvent(&event);
    return 0;
}

void schedule_refresh(int delay) {
    SDL_AddTimer(delay, sdl_refresh_timer_cb, nullptr);
}

int ffplay() {
    MediaState mediaState;
    assert(SDLPlayer::getPlayer()->setMediaState(&mediaState));

    duobei::HttpFile httpFile;
    std::string url = "http://vodkgeyttp8.vod.126.net/cloudmusic/IjAyMCAgMyEgYDEwIDhhMg==/mv/5619601/79bd07d7bd9394871da0c324d53f48dd.mp4?wsSecret=daadca716d9872c09b8d2dcbb780587f&wsTime=1573551857";
    url = "/Users/guochao/Downloads/5_往后余生.webm";
    url = "/Users/guochao/Downloads/2_告白气球.mkv";
    url = "/Users/guochao/Downloads/out.mkv";

    int ret = httpFile.Open(url);

    if (ret != duobei::FILEOK) {
        std::cout << "url is error" << std::endl;
        return -1;
    }
    // todo: 解封装 保存h264流有问题，需要av_bitstream_filter_init给h264裸流添加sps pps
    IOBufferContext ioBufferContext(0);
    httpFile.setIOBufferContext(&ioBufferContext);
    Demuxer demuxer;
    httpFile.startRead();

    std::thread readthread = std::thread([&] {
        bool status = false;
        void* param = nullptr;
        do {
            ioBufferContext.OpenInput();
            param = (AVFormatContext*)ioBufferContext.getFormatContext(&status);
        } while (!status);
        demuxer.Open(param, mediaState);
        while (1) {
            if (demuxer.ReadFrame(mediaState) == Demuxer::ReadStatus::EndOff) {
                break;
            }
        }
    });

    schedule_refresh(40);
    mediaState.audioState.decode = std::thread(&AudioState_1::audio_thread, &mediaState.audioState);
    mediaState.videoState.decode = std::thread(&VideoState_1::video_thread, &mediaState.videoState);
    SDLPlayer::getPlayer()->EventLoop(mediaState);
    ioBufferContext.io_sync.exit = true;
    if (readthread.joinable()) {
        readthread.join();
    }
    httpFile.exit = true;
    httpFile.Close();
    std::cout << "curl file end" << std::endl;
    return 0;
}