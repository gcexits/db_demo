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
    int ret = httpFile.Open("http://v6-dy.ixigua.com/ba6622f0078d51832cbdb43084559908/5d556e2f/video/m/22023f89fa74a4b40aeb39992ead46eac1c11632091000005db1d2d4ef02/?rc=amRscTh0NnF4bzMzO2kzM0ApcHpAb0k8NDo0NTU0NDk5NDg7PDNAKTZkM2hpNTQ7NDc8Nzc4PDRnKXUpQGczdSlAZjN1KTU0ZGBlcjAuMWE1Ml8tLWMtL3NzYmJebyM1Mi8zMy0yLS0wMC8uLS4vaTE0NC0vNmJjL2IwNGAxM146YzpiMHAjOmEtcCM6YDUuOg%3D%3D");
//    int ret = httpFile.Open("http://vodkgeyttp8.vod.126.net/cloudmusic/MDAwICAhITMwOTAwMDAwOA==/mv/304279/88f4918de91e55cc1a9889191f553ff1.mp4?wsSecret=343666b79ca4450d23c11299ce339c65&wsTime=1565775156");
//    std::string url = "https://www.youtube.com/watch\\?v\\=L6joGUdc6y4";
//    std::cout << url << std::endl;
//    int ret = httpFile.Open("https://www.youtube.com/watch\\?v\\=L6joGUdc6y4");
    // todo: 目前ffmpeg没支持解封装 不晓得为什么
//    int ret = httpFile.Open("https://www.sample-videos.com/video123/mp4/720/big_buck_bunny_720p_30mb.mp4");

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
              break;
          }
      }
    });
    SDLPlayer::getPlayer()->EventLoop();
    ioBufferContext.io_sync.exit = true;
    if (readthread.joinable()) {
        readthread.join();
    }
    httpFile.Close();
    std::cout << "curl file end" << std::endl;
    return 0;
}