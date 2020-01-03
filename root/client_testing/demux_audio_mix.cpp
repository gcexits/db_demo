#include "demux_audio_mix.h"

// todo :归一化混音
void mix2(char *sourseFile1, char *sourseFile2, char *objectFile, int len) {
    int const MAX = 32767;
    int const MIN = -32768;

    double f = 1;
    int output;
    int i = 0;
    for (i = 0; i < len / sizeof(short); i++) {
        int temp = 0;
        temp += *(short *)(sourseFile1 + i * 2);
        temp += *(short *)(sourseFile2 + i * 2);

        output = (int)(temp * f);
        if (output > MAX) {
            f = (double)MAX / (double)(output);
            output = MAX;
        }
        if (output < MIN) {
            f = (double)MIN / (double)(output);
            output = MIN;
        }
        if (f < 1) {
            f += ((double)1 - f) / (double)32;
        }
        *(short *)(objectFile + i * 2) = (short)output;
    }
}

int demux_audio_mix(Argument &cmd) {
    std::ofstream fp_pcm(cmd.param.mix.dst_pcm, std::ios::out | std::ios::ate);

    Api api;
    api.start(cmd.param.mix.src_audio_1, cmd.param.mix.src_audio_2);

    auto *buf_mix_1 = new uint8_t[1024 * 1024];
    auto *buf_mix_2 = new uint8_t[1024 * 1024];
    auto *buf_out = new uint8_t[1024 * 1024];
    int len1 = 0;
    int len2 = 0;

    while (1) {
        if (len1 != -1) {
            len1 = cache_1.popCache(buf_mix_1);
        }
        if (len2 != -1) {
            len2 = cache_2.popCache(buf_mix_2);
        }
        auto len = std::max(len1, len2);
        assert(len <= 640);
        if (len == -1 && api.thread_over()) {
            break;
        }
        mix2((char *)buf_mix_1, (char *)buf_mix_2, (char *)buf_out, len);
        fp_pcm.write((char *)buf_out, len);
    }

    api.stop();

    delete[] buf_mix_1;
    delete[] buf_mix_2;
    delete[] buf_out;

    fp_pcm.close();
    return 0;
}