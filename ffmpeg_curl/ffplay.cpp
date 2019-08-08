#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
}

#include "HttpFile.h"

#include <fstream>

int main() {
    std::ofstream fp;
    fp.open("./test.mp4", std::ios::ate | std::ios::binary | std::ios::out);
    uint8_t *buf = new uint8_t[10 * 512 * 1024];
    duobei::HttpFile httpFile;
    httpFile.Open("http://vodkgeyttp8.vod.126.net/cloudmusic/IGAwYDAwMTchIjA1IjIgIg==/mv/302093/3489ca539bab3c019102e36f653be7df.mp4?wsSecret=99040fdcf9b2cc5b011191e5c4623112&wsTime=1565260858");
    while (httpFile.Read(buf, 10 * 512 * 1024, 10 * 512 * 1024) != duobei::FILEEND) {
        fp.write((char *)buf, 10 * 512 * 1024);
    }
    httpFile.Close();
    delete []buf;
    fp.close();
    return 0;
}