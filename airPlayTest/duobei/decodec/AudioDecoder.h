#pragma once

#include "PlayInternal.h"

namespace duobei::audio {

class AudioDecoder {
public:
    PlayInternal play_internal;

    int Decode(uint16_t *buf, uint32_t size, bool raw, uint32_t timestamp);

    bool Init(const std::string &stream_id);
    bool Destroy();
};
}