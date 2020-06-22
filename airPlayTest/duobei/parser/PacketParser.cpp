#include "PacketParser.h"

namespace duobei::parse_ {

PacketParser::PacketParser() {
    video_invoke_ = &DecoderSpan::Video;
    audio_invoke_ = &DecoderSpan::Audio;
}

int PacketParser::Decoding(uint8_t *buffer, uint32_t length, bool key, uint32_t timestamp) {
    assert(video_invoke_);
    return (span.*video_invoke_)(buffer, length, timestamp);
}

int PacketParser::decodeH264Data(uint8_t* data, int data_len, int data_type, uint64_t pts) {
#if 0
    // todo: 当分辨率发生转换的时候会崩溃，仅把sps pps送给解码器会解码失败
    if (data_type == 0) {
        ret = span.videoDecoder.resetContext(data, data_len);
        if (ret < 0) {
            return ret;
        }
    }
    return Decoding(data, data_len, data_type == 0, pts);
#else
    if (data_type == 0) {
        keyFrame.data = new uint8_t[data_len];
        memcpy(keyFrame.data, data, data_len);
        keyFrame.length = data_len;
    } else {
        if (keyFrame.length != 0) {
            int len = data_len + keyFrame.length;
            auto *data_ = new uint8_t[len];
            memcpy(data_, keyFrame.data, keyFrame.length);
            memcpy(data_ + keyFrame.length, data, data_len);
            keyFrame.length = 0;
            delete [] keyFrame.data;
            auto ret = Decoding(data_, len, true, pts);
            delete [] data_;
            return ret;
        } else {
            return Decoding(data, data_len, false, pts);
        }
    }
#endif
}

int PacketParser::dealPcmData(uint16_t* data, int data_len, bool raw, uint64_t pts) {
    assert(audio_invoke_);
    return (span.*audio_invoke_)(data, data_len, raw, pts);
}

void PacketParser::Init(std::string &uid) {
    span.Init(uid);
}

}
