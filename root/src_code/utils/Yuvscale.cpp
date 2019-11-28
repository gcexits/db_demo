#include <iostream>

#include "Yuvscale.h"

bool YuvScaler::I420Mirror(uint8_t *src, int srcWidth, int srcHeight, uint8_t *dst, int type) {
    // todo: type 1：垂直翻转 2：水平翻转
    if (type == 1) {
        int halfWidth = srcWidth / 2;
        int yLineDataSize = srcWidth;
        int i420Index = 0, mirrorI420Index = 0;
        i420Index += srcWidth * srcHeight - yLineDataSize;
        //mirror y
        for (int h = 0; h < srcHeight; h++) {
            memcpy(dst + mirrorI420Index, src + i420Index, yLineDataSize);
            mirrorI420Index += yLineDataSize;
            i420Index -= yLineDataSize;
        }
        int uvLineDataSize = halfWidth;
        //mirror uv
        int uHeight = srcHeight / 2;
        //point to final line of u
        i420Index += (srcWidth * srcHeight * 5 / 4 + yLineDataSize - uvLineDataSize);
        for (int h = 0; h < uHeight; h++) {
            memcpy(dst + mirrorI420Index, src + i420Index, uvLineDataSize);
            mirrorI420Index += uvLineDataSize;
            i420Index -= uvLineDataSize;
        }
        //point to final line of v
        i420Index += srcWidth * srcHeight / 2;
        int vHeight = srcHeight / 2;
        for (int h = 0; h < vHeight; h++) {
            memcpy(dst + mirrorI420Index, src + i420Index, uvLineDataSize);
            mirrorI420Index += uvLineDataSize;
            i420Index -= uvLineDataSize;
        }
    } else if (type == 2) {
        int yLineStartIndex = 0;
        int uLineStartIndex = srcWidth * srcHeight;
        int vLineStartIndex = srcWidth * srcHeight * 5 / 4;
        for (int h = 0; h < srcHeight; h++) {
            for (int w = 0; w < srcWidth; w += 2) {
                dst[yLineStartIndex + w] = src[yLineStartIndex + srcWidth - w];
                dst[yLineStartIndex + w + 1] = src[yLineStartIndex + srcWidth - w - 1];
                if ((h & 1) == 0) {
                    dst[uLineStartIndex + (w >> 1)] = src[uLineStartIndex + ((srcWidth - w) >> 1)];
                    dst[vLineStartIndex + (w >> 1) + 1] = src[vLineStartIndex +
                                                              ((srcWidth - w) >> 1) - 1];
                }
            }
            yLineStartIndex += srcWidth;
            if ((h & 1) == 0) {
                uLineStartIndex += srcWidth >> 1;
                vLineStartIndex += srcWidth >> 1;
            }
        }
    } else {
        return false;
    }
    return true;
}

bool YuvScaler::YuvRote(uint8_t *src, int srcWidth, int srcHeight, int srcType, int &rotWidth, int &rotHeight, uint8_t *dst, int rot, bool need_mirror) {
    int type = 1;
    int size = srcWidth * srcHeight * 3 / 2;
    uint8_t *I420 = new uint8_t[size];
    int size_frame = srcWidth * srcHeight;
    uint8_t *dst_y = I420;
    int dst_y_stride = srcWidth;
    uint8_t *dst_u = I420 + size_frame;
    int dst_u_stride = (srcWidth + 1) / 2;
    uint8_t *dst_v = I420 + size_frame + dst_u_stride * ((srcHeight + 1) / 2);
    int dst_v_stride = (srcWidth + 1) / 2;
    int ret = 0;
    if (srcType == 1) {
        ret = libyuv::NV12ToI420(src, srcWidth, src + srcWidth * srcHeight, (srcWidth + 1) / 2 * 2, dst_y, dst_y_stride, dst_u, dst_u_stride, dst_v, dst_v_stride, srcWidth, srcHeight);
    } else if (srcType == 2) {
        ret = libyuv::NV21ToI420(src, srcWidth, src + srcWidth * srcHeight, (srcWidth + 1) / 2 * 2, dst_y, dst_y_stride, dst_u, dst_u_stride, dst_v, dst_v_stride, srcWidth, srcHeight);
    } else {
        memcpy(I420, src, size);
    }
    if (ret != 0) {
        delete[] I420;
        return false;
    }

    enum libyuv::RotationMode rmode;
    switch (rot) {
        case 0:
            rmode = libyuv::kRotate0;
            type = 2;
            break;
        case 90:
            rmode = libyuv::kRotate90;
            type = 1;
            break;
        case 180:
            rmode = libyuv::kRotate180;
            type = 2;
            break;
        case 270:
            rmode = libyuv::kRotate270;
            type = 1;
            break;
        default:
            delete[] I420;
            return false;
    }
    if (rmode == libyuv::kRotate0) {
        rotWidth = srcWidth;
        rotHeight = srcHeight;
        if (need_mirror) {
            ret = I420Mirror(I420, rotWidth, rotHeight, dst, type);
            if (!ret) {
                delete[] I420;
                return false;
            }
        } else {
            memcpy(dst, I420, size);
        }
        delete[] I420;
        return true;
    }

    int I420_Y_Size = srcWidth * srcHeight;
    int I420_U_Size = srcWidth * srcHeight / 4;

    int Dst_Stride_Y_rotate = 0;
    int Dst_Stride_U_rotate = 0;
    int Dst_Stride_V_rotate = 0;

    int Dst_Stride_Y = srcWidth;
    int Dst_Stride_U = srcWidth >> 1;
    int Dst_Stride_V = Dst_Stride_U;

    uint8_t *Y_data_Dst_rotate = dst;
    uint8_t *U_data_Dst_rotate = dst + I420_Y_Size;
    uint8_t *V_data_Dst_rotate = dst + I420_Y_Size + I420_U_Size;

    if (rmode == libyuv::kRotate90 || rmode == libyuv::kRotate270) {
        Dst_Stride_Y_rotate = srcHeight;
        Dst_Stride_U_rotate = (srcHeight + 1) >> 1;
        Dst_Stride_V_rotate = Dst_Stride_U_rotate;
    } else {
        Dst_Stride_Y_rotate = srcWidth;
        Dst_Stride_U_rotate = (srcWidth + 1) >> 1;
        Dst_Stride_V_rotate = Dst_Stride_U_rotate;
    }
    uint8_t *Y_data_src = I420;
    uint8_t *U_data_src = I420 + I420_Y_Size;
    uint8_t *V_data_src = I420 + I420_Y_Size + I420_U_Size;

    Y_data_src = I420;
    U_data_src = I420 + I420_Y_Size;
    V_data_src = I420 + I420_Y_Size + I420_U_Size;

    ret = libyuv::I420Rotate(Y_data_src, Dst_Stride_Y,
                             U_data_src, Dst_Stride_U,
                             V_data_src, Dst_Stride_V,
                             Y_data_Dst_rotate, Dst_Stride_Y_rotate,
                             U_data_Dst_rotate, Dst_Stride_U_rotate,
                             V_data_Dst_rotate, Dst_Stride_V_rotate,
                             srcWidth, srcHeight, rmode);
    if (ret != 0) {
        delete[] I420;
        return false;
    }
    delete[] I420;

    if (rmode == libyuv::kRotate90 || rmode == libyuv::kRotate270) {
        rotWidth = srcHeight;
        rotHeight = srcWidth;
    } else {
        rotWidth = srcWidth;
        rotHeight = srcHeight;
    }

    if (need_mirror) {
        int rot_size = rotWidth * rotHeight * 1.5;
        uint8_t *mirror = new uint8_t[rot_size];
        ret = I420Mirror(dst, rotWidth, rotHeight, mirror, type);
        if (!ret) {
            delete[] mirror;
            return false;
        }
        memcpy(dst, mirror, rot_size);
        delete[] mirror;
    }
    return true;
}

bool YuvScaler::I420Crop(uint8_t *src, int srcWidth, int srcHeight, int dstWidth, int dstHeight, uint8_t *crop_dst, int rot, int &cropWidth, int &cropHeight) {
    if (rot == 90 || rot == 270) {
        std::swap(dstWidth, dstHeight);
    }
    cropWidth = dstWidth;
    cropHeight = dstHeight;
    int cropped_src_width =
            std::min(srcWidth, dstWidth * srcHeight / dstHeight);
    int cropped_src_height =
            std::min(srcHeight, dstHeight * srcWidth / dstWidth);
    int src_offset_x = ((srcWidth - cropped_src_width) / 2) & ~1;
    int src_offset_y = ((srcHeight - cropped_src_height) / 2) & ~1;

    int I420_Y_Size = srcWidth * srcHeight;
    int I420_U_Size = srcWidth * srcHeight / 4;

    uint8_t *Y_data_src = src;
    uint8_t *U_data_src = Y_data_src + I420_Y_Size;
    uint8_t *V_data_src = Y_data_src + I420_Y_Size + I420_U_Size;
    int src_stride_Y = srcWidth;
    int src_stride_U = (srcWidth + 1) >> 1;
    int src_stride_V = src_stride_U;

    int dest_I420_Y_Size = dstWidth * dstHeight;
    int dest_I420_U_Size = dstWidth * dstHeight / 4;
    uint8_t *Y_data_dest = crop_dst;
    uint8_t *U_data_dest = Y_data_dest + dest_I420_Y_Size;
    uint8_t *V_data_dest = Y_data_dest + dest_I420_Y_Size + dest_I420_U_Size;
    int dest_stride_Y = dstWidth;
    int dest_stride_U = (dstWidth + 1) >> 1;
    int dest_stride_V = dest_stride_U;

    uint8_t *y_ptr =
            Y_data_src +
            src_offset_y * src_stride_Y +
            src_offset_x;
    uint8_t *u_ptr =
            U_data_src +
            src_offset_y / 2 * src_stride_U +
            src_offset_x / 2;
    uint8_t *v_ptr =
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
            libyuv::kFilterNone);
    if (ret != 0) {
        return false;
    }
    return true;
}