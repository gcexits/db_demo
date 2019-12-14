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

int main(int argc, char *argv[]) {
    Argument cmd;
    cmd.LoadProfile();
    cmd.RegisterPlayer();

    switch (cmd.param.launch) {
        case 1:
            return ffplay(cmd);
        case 2:
            return ffmpeg_capture();
        case 3:
            return flv_player(cmd);
        case 4:
            return c_test();
        case 5:
            return send_speex();
        case 6:
            return send_h264();
        case 7:
            return deal_yuv();
        case 8:
            return mainMix();
        case 9:
            return demux_audio_mix();
        case 10:
            return simplest_ffmpeg_video_filter();
        case 11:
            return main_json();
        default:
            abort();
    }
}