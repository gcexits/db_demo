#include "PacketParser.h"

namespace duobei {
namespace parse {
void PacketParser::DecodeH264Frame(Frame &frame_) {
    assert(span && video_invoke_);
    if (frame_.ret == kPpsSps) {
        frame_.pair.first = true;
        spsParser.ParseVideoSize((char *)frame_.pair.buffer, frame_.pair.length, &videoSize.width, &videoSize.height);
    } else if (frame_.ret == kKeyVideo) {
        if (!frame_.pair.first) {
            return;
        } else {
            Decoding(frame_.pair.buffer, frame_.pair.length, true, frame_.timestamp);
        }
        frame_.pair.first = true;
        frame_.pair.length = 0;
    } else if (frame_.ret == kFullVideo) {
        if (frame_.pair.first) {
            Decoding(frame_.pair.buffer, frame_.pair.length, false, frame_.timestamp);
            frame_.pair.length = 0;
        }
    }
}

int PacketParser::ParseH264Buffer(Frame &frame_) {
    uint8_t *nalu_buffer = frame_.body_.buffer;
    if (nalu_buffer[0] == 0x17) {
        if (nalu_buffer[1] == 0x00) {
            return nalu_helper.CheckPPSSPS(frame_);
        } else if (nalu_buffer[1] == 0x01) {
            return nalu_helper.CheckNalu(frame_, kKeyVideo);
        }
    } else if (nalu_buffer[0] == 0x27) {
        if (!frame_.pair.first) return kFullVideo;
        if (nalu_buffer[1] == 0x01) {
            return nalu_helper.CheckNalu(frame_, kFullVideo);
        }
    }
    return kDontCare;
}

bool PacketParser::ParseVideoCodecLayer(const uint8_t *buffer, int length) {
    has_idr = false;
    headerLength = 0;
    sps.data = buffer+headerLength;
    int spsEnd = headerLength;
    pps.data = buffer+headerLength;
    int ppsEnd = headerLength;
    int naluLength = 0;
    if (video::SEI(buffer + headerLength, naluLength)) {
        headerLength += naluLength;
        for (; headerLength + 3 < length; ++headerLength) {
            if (video::isNAL(buffer + headerLength, naluLength)) {
                ppsEnd = headerLength;
                sps.data = buffer+headerLength;
                break;
            }
        }
    }
    naluLength = 0;
    if (video::SPS(buffer + headerLength, naluLength)) {
        headerLength += naluLength;
        sps.data = buffer+headerLength;
        for (; headerLength + 4 < length; ++headerLength) {
            if (video::isNAL(buffer + headerLength, naluLength)) {
                spsEnd = headerLength;
                break;
            }
        }

        if (buffer+spsEnd > sps.data && headerLength + 4 < length && video::PPS(buffer + headerLength, naluLength)) {
            headerLength += naluLength;
            pps.data = buffer+headerLength;
            for (; headerLength + 3 < length; ++headerLength) {
                if (video::isNAL(buffer + headerLength, naluLength)) {
                    has_idr = video::IDR(buffer + headerLength, naluLength);
                    ppsEnd = headerLength;
                    break;
                }
            }
        }

        //WriteDebugLog("sps[%d:%d] pps[%d:%d] %x %x %x %x", spsBegin, spsEnd, ppsBegin, ppsEnd, buffer[i - 1], buffer[i], buffer[i + 1], buffer[i + 2]);
        if (buffer+spsEnd > sps.data && buffer+ppsEnd > pps.data) {
            sps.size = buffer+spsEnd - sps.data;
            pps.size = buffer+ppsEnd - pps.data;
#if defined(SEI_CHECK)
            if (video::SEI(buffer + headerLength, naluLength)) {
                auto sei_postion = headerLength-naluLength-1;
                auto sei_end = sei_postion;
                for (++headerLength; headerLength + 3 < length; ++headerLength) {
                    if (video::isNAL(buffer + headerLength, naluLength)) {
                        sei_end = headerLength;
                        break;
                    }
                }
                auto len =  sei_end == sei_postion?128:sei_end - sei_postion;
                print_buf((void*)(buffer + sei_postion), len);

                auto output = std::ofstream("hello.h264", std::ios::binary);
                output.write((char*)(buffer + sei_postion), len);

                WriteDebugLog("================ buffer_len=%d, sei_end=%d, sei_len=%d", length, sei_end, len);
                abort();
            }
#endif
            if (video::isNAL(buffer + headerLength, naluLength)) {
                has_idr = video::IDR(buffer + headerLength, naluLength);
                headerLength += naluLength;
            }
            return true;
        }
    }
    if (video::isNAL(buffer + headerLength, naluLength)) {
        has_idr = video::IDR(buffer + headerLength, naluLength);
        headerLength += naluLength;
    }
    return false;
}

void PacketParser::Decoding(uint8_t *buffer, uint32_t length, bool key, uint32_t timestamp) {
    assert(video_invoke_);
    assert(span);
    (span->*video_invoke_)(buffer, length, key, timestamp, m_.s);
    if (__count__ % 15 == 0 || key) {
        auto enabled_video = span->decoder_.enabled._video(key);
    }
}

void PacketParser::Audio(char* data, uint32_t size, uint32_t timestamp) {
    span->Audio(data, size, timestamp, m_.s);
}

}
}
