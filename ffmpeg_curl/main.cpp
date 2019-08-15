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
    int ret = httpFile.Open("http://v9-dy-x.ixigua.com/bcd1057268b87ff141e7bb26ecf8b41c/5d5570e1/video/m/220c9e1ac968bca4915bbf9699645c05dd2116317f350000a07e17560e15/?rc=amw7ZHU3ZWpwbzMzZ2kzM0ApcHpAb0lJNTQzNjY0NDc7NDg7PDNAKTY6ZDozNDU7Ozc4NzRlN2RnKXUpQGczdSlAZjN1KTU0ZGpvXnAxX2tjM18tLTAtMHNzYmJebyNBMS0vLS0yLS0xMS8uLS4vaV8wXy8uLjEzLzJjXy0vLTI6YzpiMHAjOmEtcCM6YDUuOg%3D%3D");
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