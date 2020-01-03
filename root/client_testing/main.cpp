#include "deal_yuv.h"
#include "demux_audio_mix.h"
#include "simplest_ffmpeg_video_filter.h"
#include "json.h"
#include "c++_test.h"
#include "H264HardDecode.h"

#include "mainPlayer.h"

int main(int argc, char *argv[]) {
    Argument cmd;
    cmd.LoadProfile();
    cmd.RegisterPlayer();
    switch (cmd.param.launch) {
        case 1:
            return ffplay(cmd);
        case 2:
            return ffmpeg_capture(cmd);
        case 3:
            return flv_player(cmd);
        case 4:
            return c_test();
        case 5:
            return send_speex(cmd);
        case 6:
            return send_h264(cmd);
        case 7:
            return deal_yuv(cmd);
        case 8:
            return demux_audio_mix(cmd);
        case 9:
            return simplest_ffmpeg_video_filter();
        case 10:
            return main_json(cmd);
        default:
            abort();
    }
}