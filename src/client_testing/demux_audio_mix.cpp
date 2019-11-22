#include "demux_audio_mix.h"

int demux_audio_mix() {
#if 0
    std::string outUrl = "./44100_2_s16le.pcm";
    std::ofstream fp_pcm;
    fp_pcm.open(outUrl, std::ios::out | std::ios::app);
    std::string url_1 = "../audio_video/1_意外.mp3";
    std::string url_2 = "../audio_video/2_盗将行.mp3";

    Api api;
    api.start(url_1, url_2);

    uint8_t *buf_mix_1 = new uint8_t[1024 * 1024];
    uint8_t *buf_mix_2 = new uint8_t[1024 * 1024];
    uint8_t *buf_out = new uint8_t[1024 * 1024];

    while (1) {
        int buf_len_1 = cache_1.popCache(buf_mix_1);
        if (buf_len_1 == 0) {
            memset(buf_mix_1, 0, 1024 * 1024);
        }
        int buf_len_2 = cache_2.popCache(buf_mix_2);
        if (buf_len_2 == 0) {
            memset(buf_mix_2, 0, 1024 * 1024);
        }
        if (buf_len_1 == 0 && buf_len_2 == 0 && api.threadOver()) {
            break;
        }
        // fp_pcm.write((char *)buf_mix_2, buf_len_2);
        fp_pcm.write((char *)buf_mix_1, buf_len_1);
    }

    api.stop();

    delete[] buf_mix_1;
    delete[] buf_mix_2;
    delete[] buf_out;

    // av_log_set_level(AV_LOG_QUIET);

    // std::cout << "audio_video duration(second) : " << demuxer.getDuration() << std::endl;

    // std::cout << "audio frame_size(nb_samples 1 channel) : " << demuxer.getAudioSize() << std::endl;

    fp_pcm.close();
#endif
    return 0;
}