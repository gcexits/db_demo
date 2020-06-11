#include "deal_yuv.h"
#include "mainPlayer.h"

int dstWidth = 200;
int dstHeight = 150;

int deal_yuv(Argument& cmd) {
    bool need_mirror = true;
    std::ifstream fp_in;
    int srcWidth = 640;
    int srcHeight = 360;
    fp_in.open(cmd.param.yuv, std::ios::binary | std::ios::in);

    // todo: 旋转
    int src_size = srcWidth * srcHeight * 1.5;
    int dst_size = dstWidth * dstHeight * 1.5;
    uint8_t *src = new uint8_t[src_size];
    uint8_t *rote = new uint8_t[src_size];
    uint8_t *crop_dst = new uint8_t[dst_size];
    YuvScaler yuvScaler;
    while (!fp_in.eof()) {
        fp_in.read((char *)src, src_size);
        int roteWidth = 0;
        int roteHeight = 0;
        int rot = 180;
        int cropWidth = 0;
        int cropHeight = 0;

        bool ret = false;
        ret = yuvScaler.YuvRote(src, srcWidth, srcHeight, 3, roteWidth, roteHeight, rote, rot, need_mirror);
        if (!ret) {
            return 0;
        }
        ret = yuvScaler.I420Crop(rote, roteWidth, roteHeight, dstWidth, dstHeight, crop_dst, rot, cropWidth, cropHeight);
        if (!ret) {
            return 0;
        }
        if (rot == 90 || rot == 270) {
            playFrameData(crop_dst, dstHeight, dstWidth, dstHeight);
        } else {
            playFrameData(crop_dst, dstWidth, dstHeight, dstWidth);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }
    fp_in.close();
    std::cout << "finish deal_yuv" << std::endl;
    return 0;
}