#include <iostream>
#include <fstream>

#if defined(_WIN32)
#include "libVCamMicSource.h"
#else
#endif

class IOVideoDevice {
public:
    virtual void start() {
        std::cout << "IOVideoDevice" << std::endl;
    }
};

class IOVideoSampleDevice : IOVideoDevice {
public:
    void start() override {
        IOVideoDevice::start();
        std::cout << "IOVideoSampleDevice" << std::endl;
    }
};

int main(int argc, char *argv[]) {
    std::cout << "this is virtual Camera project" << std::endl;
    int size = 640 * 360 * 3;
    int ret = 0;
    auto buf = new char[size];
    std::ifstream fp;
    fp.open("/Users/guochao/Downloads/2_告白气球_640x360_rgb24.rgb", std::ios::in | std::ios::binary);
    int count = 0;
#if defined(_WIN32)
    void *handle = nullptr;
    ret = VCamSource_Create(&handle);
    if (ret < 0) {
        std::cout << "VCamSource_Create failed" << std::endl;
        return -1;
    }
    ret = VCamSource_Init(handle);
    if (ret < 0) {
        std::cout << "VCamSource_Init failed" << std::endl;
        return -1;
    }
#eles
#endif
    while (count < 1000) {
        if (fp.eof()) {
            fp.seekg(0, std::ios::beg);
            count++;
        }
        fp.read(buf, size);
#if defined(_WIN32)
        ret = VCamSource_FillFrame(handle, buf, size, 640, 360);
        if (ret < 0) {
            std::cout << "VCamSource_FillFrame failed" << std::endl;
            return -1;
        }
#else
#endif
    }
#if defined(_WIN32)
    VCamSource_Destroy(&handle);
#else
#endif
    std::cout << "virtual Camera Destroy" << std::endl;
    fp.close();
    return 0;
}