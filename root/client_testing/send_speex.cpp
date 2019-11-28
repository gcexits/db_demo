#include "send_speex.h"

#include <fstream>
#include <thread>

using namespace std::placeholders;

int send_speex() {
    duobei::Time::Timestamp time;
    time.Start();
    duobei::audio::AudioEncoder audioEncoder(false);
    duobei::RtmpObject rtmpObject("rtmp://utx-live.duobeiyun.net/live/guochao");
    audioEncoder.encoder_->output_fn_ = std::bind(&duobei::RtmpObject::sendAudioPacket, rtmpObject, _1, _2, _3);

    std::string url = "/Users/guochao/Downloads/s16le_16000_1.pcm";
    std::ifstream fp_in(url, std::ios::in);
    if (!fp_in.is_open()) {
        return -1;
    }
    char *buf_1 = new char[320];
    while (!fp_in.eof()) {
        time.Stop();
        fp_in.read(buf_1, 320);
        audioEncoder.Encode(buf_1, 320, time.Elapsed());
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    delete []buf_1;
    return 0;
}