#include "Demuxer.h"
#include "HttpFile.h"
#include "VideoDisplay.h"
#include "Optional.h"

#include "../root/src_code/hlring/RingBuffer.h"
#include "../root/src_code/hlring/rbuf.h"

void RegisterPlayer() {
    using namespace std::placeholders;

    SDLPlayer* player = SDLPlayer::getPlayer();
    AVRegister::setinitVideoPlayer(std::bind(&SDLPlayer::openVideo, player, _1, _2));
    AVRegister::setinitPcmPlayer(std::bind(&SDLPlayer::openAudio, player, _1, _2));
}

int main(int argc, char* argv[]) {
    RegisterPlayer();

    duobei::HttpFile httpFile;
    std::string url = "http://vodkgeyttp8.vod.126.net/cloudmusic/IjAyMCAgMyEgYDEwIDhhMg==/mv/5619601/79bd07d7bd9394871da0c324d53f48dd.mp4?wsSecret=daadca716d9872c09b8d2dcbb780587f&wsTime=1573551857";
    url = "/Users/guochao/Downloads/5_往后余生.webm";
    url = "/Users/guochao/Downloads/2_告白气球.mkv";

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

    std::thread readthread = std::thread([&ioBufferContext, &demuxer] {
        bool status = false;
        void* param = nullptr;
        do {
            ioBufferContext.OpenInput();
            param = (AVFormatContext*)ioBufferContext.getFormatContext(&status);
        } while (!status);
        demuxer.Open(param);
        while (1) {
            if (demuxer.ReadFrame() == Demuxer::ReadStatus::EndOff) {
                break;
            }
        }
    });

    schedule_refresh(40);
    SDLPlayer::getPlayer()->EventLoop();
    ioBufferContext.io_sync.exit = true;
    if (readthread.joinable()) {
        readthread.join();
    }
    httpFile.exit = true;
    httpFile.Close();
    std::cout << "curl file end" << std::endl;
    return 0;
}