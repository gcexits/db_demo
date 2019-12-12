#include "flv_player.h"
#include "send_h264.h"
#include "send_speex.h"
#include "deal_yuv.h"
#include "audio_mix.h"
#include "demux_audio_mix.h"
#include "simplest_ffmpeg_video_filter.h"
#include "json.h"
#include "ffmpeg_capture.h"
#include "c++_test.h"
#include "H264HardDecode.h"
#include "ffplay.h"

void RegisterPlayer() {
    using namespace std::placeholders;

    SDLPlayer *player = SDLPlayer::getPlayer();
    AVRegister::setinitVideoPlayer(std::bind(&SDLPlayer::openVideo, player, _1, _2));
    AVRegister::setinitPcmPlayer(std::bind(&SDLPlayer::openAudio, player, _1, _2));

    AVRegister::setdestroyVideoPlayer(std::bind(&SDLPlayer::closeVideo, player, _1));
    AVRegister::setdestroyPcmPlayer(std::bind(&SDLPlayer::closeAudio, player, _1));
}

int main(int argc, char *argv[]) {
    RegisterPlayer();

    return flv_player();
    return ffplay();
    return c_test();
    return send_speex();
    return send_h264();
    return deal_yuv();
    return mainMix();
    return demux_audio_mix();
    return simplest_ffmpeg_video_filter();
    return main_json();
    return ffmpeg_capture();
}