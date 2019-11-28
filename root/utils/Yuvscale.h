#include <libyuv.h>

class YuvScaler {
    bool I420Mirror(uint8_t *src, int srcWidth, int srcHeight, uint8_t *dst, int type);

public:
    bool YuvRote(uint8_t *src, int srcWidth, int srcHeight, int srcType, int &rotWidth, int &rotHeight, uint8_t *dst, int rot, bool need_mirror);

    bool I420Crop(uint8_t *src, int srcWidth, int srcHeight, int dstWidth, int dstHeight, uint8_t *crop_dst, int rot, int &cropWidth, int &cropHeight);
};