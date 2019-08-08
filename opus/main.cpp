//#include "OpusEncoder.h"
#include "SpeexEncoder.h"
int main () {
    uint8_t *pcm = new uint8_t[640];
    std::ifstream fp_read;
//    fp_read.open("/Users/guochao/DBY_code/github/leixiaohua/simplest_ffmpeg_audio_player/simplest_audio_play_sdl2/NocturneNo2inEflat_44.1k_s16le.pcm", std::ios::in);
    fp_read.open("/Users/guochao/DBY_code/ff_test/flv_parse/cmake-build-debug/haha.pcm", std::ios::in);
#if 0
    duobei::audio::OpusEncoderContext opusEncoderContext;
    opusEncoderContext.Init();
#else
    duobei::audio::SpeexEncoderContext speexEncoderContext;
    speexEncoderContext.Init();
#endif
    while (!fp_read.eof()) {
        fp_read.read((char *)pcm, 640);
#if 0
        opusEncoderContext.Encode(pcm, 320);
#else
        speexEncoderContext.eatData(pcm, 640);
#endif
    }
    speexEncoderContext.Reset();
    std::cout << "opus encode over" << std::endl;
    return 0;
}