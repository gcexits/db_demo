#include <iostream>
#include <fstream>

#include "DBApi.h"

std::ofstream fp_yuv;

duobei::AVRegister::VideoPlayer YuvDta(duobei::AVRegister::Handle handle, void* data, uint32_t size, int w, int h, uint32_t ts) {
    std::cout << "video data" << std::endl;
//    fp_yuv.write((char *)data, size);
}

void *videocallback(const std::string& id, duobei::AVRegister::VideoPlayer* f) {
    using namespace std::placeholders;
    *f = std::bind(YuvDta, _1, _2, _3, _4, _5, _6);
    auto * str = new char[64];
    memcpy(str, "123", 3);
    return str;
}

duobei::AVRegister::PcmPlayer PcmDta(duobei::AVRegister::Handle handle, void* data, uint32_t size, uint32_t ts) {
    std::cout << "pcm data" << std::endl;
}

duobei::AVRegister::Handle *audiocallback(const std::string& id, duobei::AVRegister::PcmPlayer* f) {
    using namespace std::placeholders;
    *f = std::bind(PcmDta, _1, _2, _3, _4);
    duobei::AVRegister::Handle* str = new duobei::AVRegister::Handle[64];
    memcpy(str, "123", 3);
    return static_cast<duobei::AVRegister::Handle*>(str);
}

void log_fun(int level, const char* timestamp, const char* fn, int line, const char* msg) {
    std::cout << "😭😭😭 log  level : " << level << " msg : " << msg << std::endl;
}


int main(int argc, char *argv[]) {
    using namespace std::placeholders;
    duobei::AVRegister::setinitVideoPlayer(std::bind(videocallback, _1, _2));
    duobei::AVRegister::setinitPcmPlayer(std::bind(audiocallback, _1, _2));
    duobei::RegisterCallback::setprettyLogCallback(std::bind(log_fun, _1, _2, _3, _4, _5));
    int ret = duobei::DBApi::getApi()->startApi();
    getchar();
    return 0;
}