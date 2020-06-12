#pragma once

#include <iostream>

namespace duobei::parser {

class PacketParser {
    bool isNALU(const uint8_t *buf, int &len);
    bool isSPS(const uint8_t *buf, int &len);
    bool isPPS(const uint8_t *buf, int &len);
    bool isIDR(const uint8_t *buf, int &len);
    bool isSEI(const uint8_t *buf, int &len);

public:
    int spsBegin = 0;
    int spsLength = 0;
    int ppsBegin = 0;
    int ppsLength = 0;
    int headerLength = 0;

    bool ParseH264Data(uint8_t* buffer, int length);
    PacketParser()  = default;
};
}  // namespace duobei
