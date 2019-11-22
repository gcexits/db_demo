#include "ffmpeg_curl.h"
#include "flv_player.h"
#include "send_h264.h"
#include "deal_yuv.h"
#include "audio_mix.h"
#include "demux_audio_mix.h"
#include "simplest_ffmpeg_video_filter.h"
#include "json.h"

void RegisterPlayer() {
    using namespace std::placeholders;

    SDLPlayer *player = SDLPlayer::getPlayer();
    AVRegister::setinitVideoPlayer(std::bind(&SDLPlayer::openVideo, player, _1, _2));
    AVRegister::setinitPcmPlayer(std::bind(&SDLPlayer::openAudio, player, _1, _2));
}

int main(int argc, char *argv[]) {
    RegisterPlayer();

//    return ffmpeg_curl();
//    return flv_player();
//    return send_h264();
//    return deal_yuv();
//    return mainMix();
//    return demux_audio_mix();
//    return simplest_ffmpeg_video_filter();
    return main_json();
}