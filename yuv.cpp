#include <libyuv.h>
#include <fstream>
#include <iostream>

// todo: 270rot -> 比例相同，比例不同
int srcWidth = 720;
int srcHeight = 1280;

int dstWidth = 180;
int dstHeight = 320;

//int dstWidth = 180;
//int dstHeight = 320;

//int dstWidth = 480;
//int dstHeight = 640;

//int dstWidth = 640;
//int dstHeight = 480;

class YuvScaler{
public:
    bool yuvRot(uint8_t *src, int srcWidth, int srcHeight, int &rotWidth, int &rotHeight, uint8_t *dst) {
        return false;
    }
};

int main() {
    bool need_mirror = false;
    std::ifstream fp_in;
    std::ofstream fp_out;
    std::ofstream fp_rote;
    fp_in.open("/Users/guochao/DBY_code/ff_test/720x1280.yuv", std::ios::binary | std::ios::in);
    fp_rote.open("./fp_rote.yuv", std::ios::binary | std::ios::out | std::ios::app);
    fp_out.open("./scale.yuv", std::ios::binary | std::ios::out | std::ios::app);

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

    if (rmode == libyuv::kRotate0) {
        roteWidth = srcWidth;
        roteHeight = srcHeight;
        memcpy(dst, src, roteWidth * roteHeight * 1.5);
        std::cout << roteWidth << " : " << roteHeight;
    } else {
        int size = srcWidth * srcHeight * 3 / 2;

        //YUV420 image size
        int I420_Y_Size = srcWidth * srcHeight;
        int I420_U_Size = srcWidth * srcHeight / 4;

        int Dst_Stride_Y_rotate = 0;
        int Dst_Stride_U_rotate = 0;
        int Dst_Stride_V_rotate = 0;

        int Dst_Stride_Y = srcWidth;
        int Dst_Stride_U = srcWidth >> 1;
        int Dst_Stride_V = Dst_Stride_U;

        //最终写入目标
        unsigned char *Y_data_Dst_rotate = dst;
        unsigned char *U_data_Dst_rotate = dst + I420_Y_Size;
        unsigned char *V_data_Dst_rotate = dst + I420_Y_Size + I420_U_Size;

        if (rmode == libyuv::kRotate90 || rmode == libyuv::kRotate270) {
            Dst_Stride_Y_rotate = srcHeight;
            Dst_Stride_U_rotate = (srcHeight+1) >> 1;
            Dst_Stride_V_rotate = Dst_Stride_U_rotate;
        }
        else {
            Dst_Stride_Y_rotate = srcWidth;
            Dst_Stride_U_rotate = (srcWidth+1) >> 1;
            Dst_Stride_V_rotate = Dst_Stride_U_rotate;
        }
        uint8_t *Y_data_src = src;
        uint8_t *U_data_src = src + I420_Y_Size;
        uint8_t *V_data_src = src + I420_Y_Size + I420_U_Size;

        //mirro
        if(need_mirror) {
            uint8_t *Y_data_Dst_mirror = src;
            uint8_t *U_data_Dst_mirror = src + I420_Y_Size;
            uint8_t *V_data_Dst_mirror = src + I420_Y_Size + I420_U_Size;
            int Dst_Stride_Y_mirror = srcWidth;
            int Dst_Stride_U_mirror = (srcWidth+1) >> 1;
            int Dst_Stride_V_mirror = Dst_Stride_U_mirror;
            libyuv::I420Mirror(Y_data_src, Dst_Stride_Y,
                                     U_data_src, Dst_Stride_U,
                                     V_data_src, Dst_Stride_V,
                                     Y_data_Dst_mirror, Dst_Stride_Y_mirror,
                                     U_data_Dst_mirror, Dst_Stride_U_mirror,
                                     V_data_Dst_mirror, Dst_Stride_V_mirror,
                                     srcWidth, srcHeight);
            //写到这儿减少一次内存拷贝

            libyuv::I420Rotate(Y_data_Dst_mirror, Dst_Stride_Y,
                                     U_data_Dst_mirror, Dst_Stride_U,
                                     V_data_Dst_mirror, Dst_Stride_V,
                                     Y_data_Dst_rotate, Dst_Stride_Y_rotate,
                                     U_data_Dst_rotate, Dst_Stride_U_rotate,
                                     V_data_Dst_rotate, Dst_Stride_V_rotate,
                                     srcWidth, srcHeight,
                                     rmode);
        } else {
            Y_data_src = src;
            U_data_src = src + I420_Y_Size;
            V_data_src = src + I420_Y_Size + I420_U_Size;

            libyuv::I420Rotate(Y_data_src, Dst_Stride_Y,
                                     U_data_src, Dst_Stride_U,
                                     V_data_src, Dst_Stride_V,
                                     Y_data_Dst_rotate, Dst_Stride_Y_rotate,
                                     U_data_Dst_rotate, Dst_Stride_U_rotate,
                                     V_data_Dst_rotate, Dst_Stride_V_rotate,
                                     srcWidth, srcHeight,
                                     rmode);
        }

        if (rmode == libyuv::kRotate90 || rmode == libyuv::kRotate270){
            roteWidth = srcHeight;
            roteHeight = srcWidth;
            std::cout << roteWidth << " : " << roteHeight << std::endl;
        } else {
            roteWidth = srcWidth;
            roteHeight = srcHeight;
            std::cout << roteWidth << " : " << roteHeight;
        }
    }
    fp_rote.write((char *)dst, roteWidth * roteHeight * 1.5);

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

#else
    int cropped_src_width =
            std::min(roteWidth, dstWidth * roteHeight / dstHeight);
    int cropped_src_height =
            std::min(roteHeight, dstHeight * roteWidth / dstWidth);
    // Make sure the offsets are even to avoid rounding errors for the U/V planes.
    int src_offset_x = ((roteWidth - cropped_src_width) / 2) & ~1;
    int src_offset_y = ((roteHeight - cropped_src_height) / 2) & ~1;

    //YUV420 image size
    int I420_Y_Size = roteWidth * roteHeight;
    int I420_U_Size = roteWidth * roteHeight / 4;

    uint8_t *Y_data_src = nullptr;
    if (rmode == libyuv::kRotate0) {
        Y_data_src = src;
    } else {
        Y_data_src = dst;
    }
    uint8_t *U_data_src = Y_data_src + I420_Y_Size;
    uint8_t *V_data_src = Y_data_src + I420_Y_Size + I420_U_Size;
    int src_stride_Y = roteWidth;
    int src_stride_U = (roteWidth+1) >> 1;
    int src_stride_V = src_stride_U;

    int dest_I420_Y_Size = dstWidth * dstHeight;
    int dest_I420_U_Size = dstWidth * dstHeight / 4;
    uint8_t *Y_data_dest = crop_dst;
    uint8_t *U_data_dest = Y_data_dest + dest_I420_Y_Size;
    uint8_t *V_data_dest = Y_data_dest + dest_I420_Y_Size + dest_I420_U_Size;
    int dest_stride_Y = dstWidth;
    int dest_stride_U = (dstWidth+1) >> 1;
    int dest_stride_V = dest_stride_U;

    uint8_t* y_ptr =
            Y_data_src +
            src_offset_y * src_stride_Y +
            src_offset_x;
    uint8_t* u_ptr =
            U_data_src +
            src_offset_y / 2 * src_stride_U +
            src_offset_x / 2;
    uint8_t* v_ptr =
            V_data_src +
            src_offset_y / 2 * src_stride_V +
            src_offset_x / 2;

    int ret = libyuv::I420Scale(
            y_ptr,
            src_stride_Y,
            u_ptr,
            src_stride_U,
            v_ptr,
            src_stride_V,
            cropped_src_width, cropped_src_height,
            Y_data_dest,
            dest_stride_Y,
            U_data_dest,
            dest_stride_U,
            V_data_dest,
            dest_stride_V,
            dstWidth, dstHeight,
            libyuv::kFilterBilinear);
    if (ret != 0) {
        return false;
    }
    fp_out.write((char *)crop_dst, dstWidth * dstHeight * 1.5);
#endif

    fp_in.close();
    fp_out.close();
    return 0;
}