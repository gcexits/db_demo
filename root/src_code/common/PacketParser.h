#pragma once

#include <iostream>

namespace duobei {

namespace parser {
class PacketParser {
    bool isNAL(const uint8_t *buf, int &len) {
        static const uint8_t nal3[] = {0x00, 0x00, 0x01};
        if (memcmp(buf, nal3, sizeof(nal3)) == 0) {
            len = 3;
            return true;
        }

        static const uint8_t nal4[] = {0x00, 0x00, 0x00, 0x01};
        if (memcmp(buf, nal4, sizeof(nal4)) == 0) {
            len = 4;
            return true;
        }

        return false;
    }

    bool isSPS(const uint8_t *buf, int &len) {
        // !!! warning (buf[i] & 0x1f) == 0x07
        //if (buf[i++] == 0x00 && buf[i++] == 0x00 && buf[i++] == 0x00 && buf[i++] == 0x01 && (buf[i] & 0x1f) == 0x07) //sps
        //https://cardinalpeak.com/blog/the-h-264-sequence-parameter-set/
        //const uint8_t sps[] = { 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a, 0xf8, 0x41, 0xa2 };
        if (!isNAL(buf, len)) {
            return false;
        }
        return (buf[len] & 0x1f) == 0x07;
    }

    bool isPPS(const uint8_t *buf, int &len) {
        //const uint8_t pps[] = { 0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x38, 0x80 };
        if (!isNAL(buf, len)) {
            return false;
        }
        return (buf[len] & 0x1f) == 0x08;
    }
    // is I frame
    bool isIFM(const uint8_t *buf, int &len) {
        if (!isNAL(buf, len)) {
            return false;
        }
        return (buf[len] & 0x1f) == 0x05;
    }

public:
    // is SEI
    bool isSEI(const uint8_t *buf, int &len) {
        if (!isNAL(buf, len)) {
            return false;
        }
        return buf[len] == 0x06;
    }
    int spsBegin = 0;
    int spsLength = 0;
    int ppsBegin = 0;
    int ppsLength = 0;
    int headerLength = 0;
    bool SPSPPS(uint8_t *buffer, int length);
    int dumpHeader(uint8_t *buffer, int length);
    PacketParser()  = default;
};
}  // namespace parser
}  // namespace duobei
