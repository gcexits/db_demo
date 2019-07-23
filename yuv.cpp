#include <fstream>
#include <iostream>

#include "yuvscale.h"

// todo: src(9:16)
int srcWidth = 720;
int srcHeight = 1280;

// todo: dst(9:16)
//int dstWidth = 180;
//int dstHeight = 320;

// todo: dst(16:9)
//int dstWidth = 320;
//int dstHeight = 180;

// todo: dst(3:4)
//int dstWidth = 480;
//int dstHeight = 640;

// todo: dst(4:3)
int dstWidth = 200;
int dstHeight = 150;

#define ROTANDCROP
//#define ROTE
//#define CROP

int main() {
    bool need_mirror = true;
    std::ifstream fp_in;
    std::ofstream fp_out;
    std::ofstream fp_rote;
    fp_in.open("/Users/guochao/DBY_code/ff_test/nv21_720x1280.yuv", std::ios::binary | std::ios::in);
    fp_rote.open("./fp_rote.yuv", std::ios::binary | std::ios::out | std::ios::ate);
    if (!fp_out.is_open()) {
        fp_out.open("./scale.yuv", std::ios::binary | std::ios::out | std::ios::ate);
    }

    // todo: 旋转
    enum libyuv::RotationMode rmode = libyuv::kRotate0;
    int frame_size = srcWidth * srcHeight * 1.5;
    int y_size = srcWidth * srcHeight;
    uint8_t *src = new uint8_t[frame_size];
    uint8_t *dst = new uint8_t[frame_size];
    uint8_t *crop_dst = new uint8_t[frame_size];
    fp_in.read((char *)src, frame_size);
    int roteWidth = 0;
    int roteHeight = 0;
    int rot = 270;
    int cropWidth = 0;
    int cropHeight = 0;

    YuvScaler yuvScaler;
    bool ret = false;
#if defined(ROTANDCROP)
    ret = yuvScaler.YuvRote(src, srcWidth, srcHeight, 2, roteWidth, roteHeight, dst, rot, need_mirror);
    if (!ret) {
        return 0;
    }
    ret = yuvScaler.I420Crop(dst, roteWidth, roteHeight, dstWidth, dstHeight, crop_dst, rot, cropWidth, cropHeight);
    if (!ret) {
        return 0;
    }
    fp_rote.write((char *)dst, roteWidth * roteHeight * 1.5);
    fp_out.write((char *)crop_dst, dstWidth * dstHeight * 1.5);
#endif

#if defined(ROTE)
    ret = yuvScaler.Nv21Rot(src, srcWidth, srcHeight, roteWidth, roteHeight, dst, rot, need_mirror);
    if (!ret) {
        return 0;
    }
    fp_rote.write((char *)dst, roteWidth * roteHeight * 1.5);
#endif

#if defined(CROP)
    ret = yuvScaler.I420Crop(src, srcWidth, srcHeight, dstWidth, dstHeight, crop_dst, rot, cropWidth, cropHeight);
    if (!ret) {
        return 0;
    }
    fp_out.write((char *)crop_dst, dstWidth * dstHeight * 1.5);
#endif

#if 0
    int crop_rotate_width = 0;
    int crop_rotate_height = 0;
    int crop_x = 0;
    int crop_y = 0;
    int crop_width = 0;
    int crop_height = 0;
    if (rmode == libyuv::kRotate0 || rmode == libyuv::kRotate180) {

    } else {
        crop_rotate_width = roteHeight;
        crop_rotate_height = roteWidth;
        if (crop_rotate_width < crop_rotate_height) {
            crop_rotate_height = crop_rotate_width / 4 * 3;
            if (crop_rotate_height % 2 != 0) {
                crop_rotate_height += 1;
            }
            crop_x = (srcWidth - crop_rotate_height) / 2;
            if (crop_x % 2 != 0) {
                crop_x += 1;
            }
        }
        crop_width = crop_rotate_height;
        crop_height = crop_rotate_width;
    }
    int size_frame = crop_rotate_width * crop_rotate_height;
    uint8_t *dst_y = crop_dst;
    int dst_y_stride = crop_rotate_width;
    uint8_t *dst_u = crop_dst + size_frame;
    int dst_u_stride = (crop_rotate_width + 1) / 2;
    uint8_t *dst_v = crop_dst + size_frame + dst_u_stride * ((crop_rotate_height + 1) / 2);
    int dst_v_stride = (crop_rotate_width + 1) / 2;

    if (crop_rotate_width == srcWidth && crop_rotate_height == srcHeight && rmode == libyuv::kRotate0) {
    } else {
        libyuv::ConvertToI420(dst, roteWidth * roteHeight * 1.5,
                                    dst_y, dst_y_stride,
                                    dst_u, dst_u_stride,
                                    dst_v, dst_v_stride,
                                    crop_x, crop_y,  // 以左上角为原点，裁剪起始点
                                    roteWidth, roteHeight,
                                    crop_width, crop_height,
                                    libyuv::kRotate0,
                                    libyuv::FOURCC_I420);
    }
    std::cout << "crop_rote " << crop_rotate_width << " : " << crop_rotate_height << std::endl;
    fp_out.write((char *)crop_dst, size_frame * 1.5);
#endif

    fp_in.close();
    fp_out.close();
    fp_rote.close();
    std::cout << "finish" << std::endl;
    return 0;
}