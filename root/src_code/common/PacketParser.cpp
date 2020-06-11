#include "PacketParser.h"

namespace duobei {
namespace parser {
bool PacketParser::SPSPPS(uint8_t *buffer, int length) {
    headerLength = 0;
    spsBegin = headerLength;
    int spsEnd = headerLength;
    ppsBegin = headerLength;
    int ppsEnd = headerLength;
    int naulLength = 0;
    if (isSPS(buffer, naulLength)) {
        headerLength += naulLength;
        spsBegin = headerLength;
        for (; headerLength + 4 < length; ++headerLength) {
            if (isNAL(buffer + headerLength, naulLength)) {
                spsEnd = headerLength;
                break;
            }
        }

        if (spsEnd > spsBegin && headerLength + 4 < length && isPPS(buffer + headerLength, naulLength)) {
            headerLength += naulLength;
            ppsBegin = headerLength;
            for (; headerLength + 3 < length; ++headerLength) {
                if (isNAL(buffer + headerLength, naulLength)) {
                    ppsEnd = headerLength;
                    break;
                }
            }
        }

        if (spsEnd > spsBegin && ppsEnd > ppsBegin) {
            spsLength = spsEnd - spsBegin;
            ppsLength = ppsEnd - ppsBegin;
            if (isNAL(buffer + headerLength, naulLength)) {
                headerLength += naulLength;
            }
            return true;
        }
    }
    if (isNAL(buffer + headerLength, naulLength)) {
        headerLength += naulLength;
    }
    return false;
}

int PacketParser::dumpHeader(uint8_t *buffer, int length) {
    int naulLength = 0;
    int index = 0;
    while (!isSPS(buffer + index, naulLength)) {
        index++;
    }
    return index;
}

bool PacketParser::ParseH264Data(uint8_t *buffer, int length) {
    has_idr = false;
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
            has_idr = true;
            if (ppsLength == 0 && ppsBegin > 0) {
                ppsLength = i - ppsBegin;
            }
            headerLength = i + len;
        } else {
            if (!isNAL(buffer + i, len)) {
                len = 1;
            }
            if (headerLength == 0) {
                headerLength = i + len;
            }
        }
        i += len;
    }

    return has_idr;
}

}  // namespace parser
}  // namespace duobei
