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
    int ret = httpFile.Open("http://vodkgeyttp8.vod.126.net/cloudmusic/ImQwISAwIGEwZGEhMjEjZA==/mv/5917012/908bd4c52da1bebb66c525de93045d1e.mp4?wsSecret=cff082a4e2b592ff7cad68ba589e64b4&wsTime=1566293200");
//    int ret = httpFile.Open("/Users/guochao/Downloads/youtube.webm");
//    int ret = httpFile.Open("/Users/guochao/Downloads/douyin_3.mp4");
//    int ret = httpFile.Open("/Users/guochao/DBY_code/ff_test/1.flv");
//    int ret = httpFile.Open("/Users/guochao/Downloads/wangyiyun.mp4");
//    int ret = httpFile.Open("https://playback2.duobeiyun.com/jzca393d0a562945259376098f6ab8a324/streams/out-video-jze002ed10fa274d69994bdea9685e8069_f_1556449161787_t_1556450882947.flv");
//    int ret = httpFile.Open("http://vodkgeyttp8.vod.126.net/cloudmusic/MDAwICAhITMwOTAwMDAwOA==/mv/304279/88f4918de91e55cc1a9889191f553ff1.mp4?wsSecret=343666b79ca4450d23c11299ce339c65&wsTime=1565775156");
//    std::string url = "https://www.youtube.com/watch\\?v\\=L6joGUdc6y4";
//    int ret = httpFile.Open("https://www.youtube.com/watch\\?v\\=L6joGUdc6y4");
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