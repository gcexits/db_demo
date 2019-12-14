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

int ffplay(Argument& cmd) {
    MediaState mediaState;
    assert(cmd.player.setMediaState(&mediaState));

    duobei::HttpFile httpFile;
    int ret = httpFile.Open(cmd.param.url);

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
        demuxer.Open(param, mediaState, cmd.player);
        while (1) {
            if (demuxer.ReadFrame(mediaState) == Demuxer::ReadStatus::EndOff) {
                break;
            }
        }
    });

    schedule_refresh(40);
    cmd.player.EventLoop(mediaState);
    ioBufferContext.io_sync.exit = true;
    if (readthread.joinable()) {
        readthread.join();
    }
    httpFile.exit = true;
    httpFile.Close();
    std::cout << "curl file end" << std::endl;
    return 0;
}