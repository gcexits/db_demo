#include "HttpFile.h"
#include "Demuxer.h"

#include "../flv_parse/hlring/RingBuffer.h"
#include "../flv_parse/hlring/rbuf.h"

void RegisterPlayer() {
    using namespace std::placeholders;

    SDLPlayer* player = SDLPlayer::getPlayer();
    AVRegister::setinitVideoPlayer(std::bind(&SDLPlayer::openVideo, player, _1, _2));
    AVRegister::setinitPcmPlayer(std::bind(&SDLPlayer::openAudio, player, _1, _2));
}

int main(int argc, char* argv[]) {
    RegisterPlayer();

    duobei::HttpFile httpFile;
    std::string url = "/Users/guochao/Downloads/4_浪子回頭.mkv";
    int ret = httpFile.Open(url);
    // todo: 目前ffmpeg没支持解封装 不晓得为什么
//    int ret = httpFile.Open("https://www.sample-videos.com/video123/mp4/720/big_buck_bunny_720p_30mb.mp4");

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
      if (!status) {
          do {
              ioBufferContext.OpenInput();
              param = (AVFormatContext*)ioBufferContext.getFormatContext(&status);
          } while (!status);
          demuxer.Open(param);
      }
      while (1) {
          if (demuxer.ReadFrame() == Demuxer::ReadStatus::EndOff) {
              SDLPlayer::getPlayer()->running = false;
              break;
          }
      }
    });
    SDLPlayer::getPlayer()->EventLoop();
    ioBufferContext.io_sync.exit = true;
    demuxer.exit = true;
    if (readthread.joinable()) {
        readthread.join();
    }
    httpFile.exit = true;
    httpFile.Close();
    std::cout << "curl file end" << std::endl;
    return 0;
}