#include <fstream>
#include <iostream>
#include <string>

// todo :混音demo

void mix2(char *sourseFile1, char *sourseFile2, char *objectFile, int len) {
    // todo :归一化混音
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

int main(int argc, char *argv[]) {
    int mix_count = 0;
    std::ifstream fp1;
    std::ifstream fp2;
    std::ofstream fp_mix;
    std::string url_1 = "../audio_video/1_mix.pcm";
    std::string url_2 = "../audio_video/2_mix.pcm";
    std::string url_3 = "./mix.pcm";

    fp1.open(url_1, std::ios::in);
    fp2.open(url_2, std::ios::in);
    fp_mix.open(url_3, std::ios::out | std::ios::app);
    char *buf_1 = new char[320];
    char *buf_2 = new char[320];
    char *buf_3 = new char[320];
    while (!fp1.eof() && !fp2.eof()) {
        fp1.read(buf_1, 320);
        fp2.read(buf_2, 320);
        mix2(buf_1, buf_2, buf_3, 320);
        fp_mix.write(buf_3, 320);
        std::cout << "mix count = " << mix_count++ << std::endl;
    }
    std::cout << "mix over, but maybe data is not read finish" << std::endl;
    fp1.close();
    fp2.close();
    fp_mix.close();
    return 0;
}