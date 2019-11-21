#pragma once

#include "librtmp/rtmp.h"

#include "../common/PacketParser.h"

#include <iostream>
#include <string>

namespace duobei {

class RtmpObject {
    RTMP *rtmp = nullptr;
    std::string url;
    duobei::parser::PacketParser parser_;

    size_t packVideoSpsPps(char *body, const uint8_t *sps, size_t sps_len, const uint8_t *pps, size_t pps_len);

    bool send_video_sps_pps(const uint8_t *sps, int sps_len, const uint8_t *pps, int pps_len, uint32_t timestamp);

    bool send_video_only(const uint8_t *buf, int len, bool bKeyFrame, uint32_t timestamp);

    bool Init();
public:
    RtmpObject(std::string url_) : url(url_) {
        Init();
    }

    bool sendH264Packet(uint8_t *buffer, int length, bool keyFrame, uint32_t timestamp, bool isdemux);
};
}  // namespace duobei