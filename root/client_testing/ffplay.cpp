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
    std::string url = "http://v1-dy.ixigua.com/dbf411f1ef218f6b158433cdd54356d4/5df24e00/video/tos/cn/tos-cn-ve-15/d738504f39fb4e658b2b3aba890c9ed6/?a=1128&br=1090&cr=0&cs=0&dr=0&ds=3&er=&l=201912122125350100140431310B6C712C&lr=aweme&qs=0&rc=M25obWdpcDc5cTMzOWkzM0ApZWk3OWg7NGVlN2c4Nmk7ZWdsMDI2ZjQxcGpfLS1iLS9zczUwNmItYjZgL2EwNDVeNjM6Yw%3D%3D";
    url = "/Users/guochao/Downloads/5_往后余生.webm";
//    url = "/Users/guochao/Downloads/2_告白气球.mkv";
//    url = "/Users/guochao/Downloads/out.mkv";

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