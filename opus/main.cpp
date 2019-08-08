#define OPUS

#if defined(OPUS)
#include "OpusEncoder.h"
#else
#include "SpeexEncoder.h"
#endif

int main () {
#if defined(OPUS)
    int frame_size = 640;
#else
    int frame_size = 320;
#endif
    uint8_t *pcm = new uint8_t[frame_size];
    std::ifstream fp_read;
//    fp_read.open("/Users/guochao/DBY_code/github/leixiaohua/simplest_ffmpeg_audio_player/simplest_audio_play_sdl2/NocturneNo2inEflat_44.1k_s16le.pcm", std::ios::in);
    fp_read.open("/Users/guochao/DBY_code/ff_test/flv_parse/cmake-build-debug/haha.pcm", std::ios::in);
#if defined(OPUS)
    duobei::audio::OpusEncoderContext opusEncoderContext;
    opusEncoderContext.Init();
#else
    duobei::audio::SpeexEncoderContext speexEncoderContext;
    speexEncoderContext.Init();
#endif
    while (!fp_read.eof()) {
        fp_read.read((char *)pcm, frame_size);
#if defined(OPUS)
        opusEncoderContext.Encode(pcm, frame_size);
#else
        speexEncoderContext.eatData(pcm, 640);
#endif
    }
#if defined(OPUS)
    opusEncoderContext.Reset();
#else
    speexEncoderContext.Reset();
#endif
    std::cout << "opus encode over" << std::endl;
    return 0;
}