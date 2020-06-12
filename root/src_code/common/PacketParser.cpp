#include "PacketParser.h"

namespace duobei::parser {

bool PacketParser::isNALU(const uint8_t* buf, int& len) {
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

bool PacketParser::isSPS(const uint8_t* buf, int& len) {
    if (!isNALU(buf, len)) {
        return false;
    }
    return (buf[len] & 0x1f) == 0x07;
}

bool PacketParser::isPPS(const uint8_t* buf, int& len) {
    if (!isNALU(buf, len)) {
        return false;
    }
    return (buf[len] & 0x1f) == 0x08;
}

bool PacketParser::isIDR(const uint8_t* buf, int& len) {
    if (!isNALU(buf, len)) {
        return false;
    }
    return (buf[len] & 0x1f) == 0x05;
}

bool PacketParser::isSEI(const uint8_t* buf, int& len) {
    if (!isNALU(buf, len)) {
        return false;
    }
    return buf[len] == 0x06;
}

bool PacketParser::ParseH264Data(uint8_t* buffer, int length) {
    headerLength = 0;
    spsBegin = 0;
    spsLength = 0;
    ppsBegin = 0;
    ppsLength = 0;
    int i = 0;
    for (; i < length - 3;) {
        int len = 0;
        if (isSPS(buffer + i, len)) {
            spsBegin = headerLength = i + len;
        } else if (isPPS(buffer + i, len)) {
            if (spsLength == 0 && spsBegin > 0) {
                spsLength = i - spsBegin;
            }
            ppsBegin = headerLength = i + len;
        } else if (isSEI(buffer + i, len)) {
            if (ppsLength == 0 && ppsBegin > 0) {
                ppsLength = i - ppsBegin;
            }
            headerLength = i + len;
        } else if (isIDR(buffer + i, len)) {
            if (ppsLength == 0 && ppsBegin > 0) {
                ppsLength = i - ppsBegin;
            }
            headerLength = i + len;
        } else {
            if (!isNALU(buffer + i, len)) {
                len = 1;
            }
            if (headerLength == 0) {
                headerLength = i + len;
            }
        }
        i += len;
    }

    return spsLength > 0 && ppsLength > 0;
}

}  // namespace duobei
