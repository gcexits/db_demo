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

}
}  // namespace duobei
