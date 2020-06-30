#pragma once

#include "../decodec/DecoderSpan.h"

namespace duobei::parse_ {

class PacketParser {
    struct KeyFrame {
        uint8_t *data;
        int length;
    };
    KeyFrame keyFrame;
    DecoderSpan span;
    DecoderSpan::InvokeVideoType video_invoke_ = nullptr;
    DecoderSpan::InvokeAudioType audio_invoke_ = nullptr;

    int Decoding(uint8_t* buffer, uint32_t length, bool isKey, uint32_t timestamp);

public:
    explicit PacketParser();
    ~PacketParser() = default;

    int decodeH264Data(uint8_t *data, int data_len, int data_type, uint64_t pts);
    int dealPcmData(uint16_t *data, int data_len, bool raw, uint64_t pts);
    void Init(std::string &uid);
};
}
