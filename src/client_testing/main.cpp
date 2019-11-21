#include "ffmpeg_curl.h"
#include "flv_player.h"
#include "send_h264.h"

void RegisterPlayer() {
    using namespace std::placeholders;

    SDLPlayer *player = SDLPlayer::getPlayer();
    AVRegister::setinitVideoPlayer(std::bind(&SDLPlayer::openVideo, player, _1, _2));
    AVRegister::setinitPcmPlayer(std::bind(&SDLPlayer::openAudio, player, _1, _2));
}

int main(int argc, char *argv[]) {
    RegisterPlayer();

    return ffmpeg_curl();
    return flv_player();
    return send_h264();
}