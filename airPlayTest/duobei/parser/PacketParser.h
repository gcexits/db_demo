#pragma once

#include "codec/DecoderSpan.h"
#include "network/RTMPObject.h"
#include "offline/FileBase.h"

#include "CompositeDevice.h"
#include "SpsParser.h"

namespace duobei {
#define LEN4_(dataTmp) ((dataTmp[0] & 0x000000FF) << 24 | (dataTmp[1] & 0x000000FF) << 16 | (dataTmp[2] & 0x000000FF) << 8 | (dataTmp[3] & 0x000000FF))
#define LEN3_(dataTmp) ((dataTmp[0] & 0x000000FF) << 16 | (dataTmp[1] & 0x000000FF) << 8 | (dataTmp[2] & 0x000000FF))
#define LEN2_(dataTmp) ((dataTmp[0] & 0x000000FF) << 8 | (dataTmp[1] & 0x000000FF))
#define LEN1_(dataTmp) ((dataTmp[0] & 0x000000FF))
#define TIME4_(dataTmp) ((dataTmp[3] & 0x000000FF) << 24 | (dataTmp[0] & 0x000000FF) << 16 | (dataTmp[1] & 0x000000FF) << 8 | (dataTmp[2] & 0x000000FF))
namespace parse {
using  Span = video::Span;

enum {
    kDontCare = 9,
    kPpsSps = 10,
    kFullVideo = 11,
    kKeyVideo = 12
};

struct NaluHelper {
    int nalu_length = -1;
    int CheckNalu(Frame& frame, int default_value) {
        uint8_t *nalu_buffer = frame.body_.buffer;
        int postion = 0;
        nalu_buffer += 5;
        video::Span nalu;
        while (postion < frame.body_.length - 5) {
            switch (nalu_length) {
                case 4:
                    nalu.size = LEN4_((&(nalu_buffer[0])));
                    break;
                case 3:
                    nalu.size = LEN3_((&(nalu_buffer[0])));
                    break;
                case 2:
                    nalu.size = LEN2_((&(nalu_buffer[0])));
                    break;
                case 1:
                    nalu.size = LEN1_((&(nalu_buffer[0])));
                    break;
                default:
                    return kDontCare;
            }
            postion += nalu_length + nalu.size;
            nalu_buffer += nalu_length;
            nalu.data = nalu_buffer;

            frame.WriteH264Header();
            if (nalu.DataEnd() > frame.BodyEnd()) {
                return kDontCare;
            }
            frame.WriteData(nalu);
            nalu_buffer += nalu.size;
        }
        return default_value;
    }
    int CheckPPSSPS(Frame& frame_) {
        video::Span sps;
        video::Span pps;
        uint8_t *nalu_buffer = frame_.body_.buffer;
        nalu_length = (nalu_buffer[9] & 3) + 1;
        int sps_numbers = nalu_buffer[10] & 0x1F;  //sps数量
        if (sps_numbers > 1) {
            WriteErrorLog("sps_numbers=%d>1", sps_numbers);
        }
        nalu_buffer += 11;
        for (int i = 0; i < sps_numbers; i++) {
            sps.data = nalu_buffer + 2;
            sps.size = LEN2_((&(nalu_buffer[0])));
            nalu_buffer += 2 + sps.size;
        }
        sps.Dump();
        int pps_numbers = LEN1_((&(nalu_buffer[0])));
        if (pps_numbers > 1) {
            WriteErrorLog("pps_numbers=%d>1", pps_numbers);
        }
        nalu_buffer += 1;
        for (int i = 0; i < pps_numbers; i++) {
            pps.data = nalu_buffer + 2;
            pps.size = LEN2_((&(nalu_buffer[0])));
            nalu_buffer += 2 + pps.size;
        }

        frame_.pair.length = 0;
        frame_.WriteH264Header();
        if (sps.DataEnd() > frame_.BodyEnd()) {
            return kDontCare;
        }
        frame_.WriteData(sps);

        frame_.WriteH264Header();
        if (pps.DataEnd() > frame_.BodyEnd()) {
            return kDontCare;
        }
        frame_.WriteData(pps);

        return kPpsSps;
    }
};

class PacketParser {
    void DecodeH264Frame(Frame& frame_);
    int ParseH264Buffer(Frame& frame_);

public:
    explicit PacketParser() {
        param = FlowHolder::New();
    }
    ~PacketParser() {
        if (spspps) {
            delete []spspps;
        }
        spsppsLength = 0;
    }

    StreamMeta m_;
    NaluHelper nalu_helper;
    uint8_t *spspps = nullptr;
    int spsppsLength = 0;
    Frame frame_;
    SpsParser spsParser;
    collect::VideoSize videoSize{0, 0};

    DecoderSpan* span = nullptr;
    DecoderSpan::InvokeType video_invoke_ = nullptr;

    void Decoding(uint8_t* buffer, uint32_t length, bool key, uint32_t timestamp);

    void Audio(char* data, uint32_t size, uint32_t timestamp);

    void setDecoder(DecoderSpan& d) {
        auto& mode = m_.s;
        span = &d;
        if (d.video_mime_type_ == MIMEType::kH264) {
            video_invoke_ = &DecoderSpan::H264;
        } else {
            if (mode == StreamType::DualHigh) {
                video_invoke_ = &DecoderSpan::High;
            } else if (mode == StreamType::DualLow) {
                video_invoke_ = &DecoderSpan::Low;
            } else {
                video_invoke_ = &DecoderSpan::Video;
            }
        }
    }

    void Parsing() {
        frame_.ret = ParseH264Buffer(frame_);
        DecodeH264Frame(frame_);
    }

    void Parsing(uint8_t* data, uint32_t len, uint32_t timestamp) {
        memcpy(frame_.body_.buffer, data, len);
        frame_.body_.length = len;
        frame_.timestamp = timestamp;
        Parsing();
    }

    video::Span sps;
    video::Span pps;

    bool has_idr = false;
    int headerLength = 0;
    bool ParseVideoCodecLayer(const uint8_t* buffer, int length);
};
}
}
