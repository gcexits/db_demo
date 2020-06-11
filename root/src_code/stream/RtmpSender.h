#pragma once

#include "librtmp/rtmp.h"

#include "../common/PacketParser.h"

#include <iostream>
#include <string>

namespace duobei {

class RtmpObject {
    enum format {
        send,
        recv
    };
    int type = -1;
    std::string url;
    duobei::parser::PacketParser parser_;

    size_t packVideoSpsPps(char *body, const uint8_t *sps, size_t sps_len, const uint8_t *pps, size_t pps_len);

    bool send_video_sps_pps(const uint8_t *sps, int sps_len, const uint8_t *pps, int pps_len, uint32_t timestamp);

    bool send_video_only(const uint8_t *buf, int len, bool bKeyFrame, uint32_t timestamp);


    bool Init(RTMPPacket *cp);
public:
    RTMP *rtmp = nullptr;
    RtmpObject(std::string url_, RTMPPacket *cp, int type_) : url(url_), type(type_) {
        Init(cp);
    }

    bool sendH264Packet(uint8_t *buffer, int length, bool keyFrame, uint32_t timestamp);
    void sendAudioPacket(const int8_t *buf, int len, uint32_t timestamp, int type);
};
}  // namespace duobei