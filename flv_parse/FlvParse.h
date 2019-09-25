#pragma once

#include <fstream>
#include <iostream>
#include <list>
#include <thread>

#include <librtmp/amf.h>

#include "H264Decoder.h"
#include "SpeexDecoder.h"

#define LEN4_(dataTmp) ((dataTmp[0] & 0x000000FF) << 24 | (dataTmp[1] & 0x000000FF) << 16 | (dataTmp[2] & 0x000000FF) << 8 | (dataTmp[3] & 0x000000FF))
#define LEN3_(dataTmp) ((dataTmp[0] & 0x000000FF) << 16 | (dataTmp[1] & 0x000000FF) << 8 | (dataTmp[2] & 0x000000FF))
#define LEN2_(dataTmp) ((dataTmp[0] & 0x000000FF) << 8 | (dataTmp[1] & 0x000000FF))
#define LEN1_(dataTmp) ((dataTmp[0] & 0x000000FF))
#define TIME4_(dataTmp) ((dataTmp[3] & 0x000000FF) << 24 | (dataTmp[0] & 0x000000FF) << 16 | (dataTmp[1] & 0x000000FF) << 8 | (dataTmp[2] & 0x000000FF))

class Header {
public:
    uint8_t* header_data = nullptr;
    Header() {
        header_data = new uint8_t[9];
    }

    ~Header() {
        delete[] header_data;
    }
};

class Body {
public:
    uint8_t* body_data = nullptr;
    Body() {
        body_data = new uint8_t[15];
    }

    ~Body() {
        delete[] body_data;
    }
};

enum {
    kDontCare = 9,
    kPpsSps = 10,
    kFullVideo = 11,
    kKeyVideo = 12
};

class FlvPlayer {
public:
    struct PositionTimestamp {
        uint32_t timestamp;  //毫秒
        size_t position;
    };
    struct PositionManager {
        //flv info
        std::string metadatacreator;
        bool hasKeyframes = false;
        bool hasVideo = false;
        bool hasAudio = false;
        bool hasMetadata = false;
        bool canSeekToEnd = false;
        double duration = 0;
        double datasize = 0;
        double videosize = 0;
        double framerate = 0;
        double videodatarate = 0;
        double videocodecid = 0;
        double width = 0;
        double height = 0;
        double audiosize = 0;
        double audiodatarate = 0;
        double audiocodecid = 0;
        double audiosamplerate = 0;
        double audiosamplesize = 0;
        bool stereo = false;
        double filesize = 0;
        double lasttimestamp = 0;
        double lastkeyframetimest = 0;
        double lastkeyframelocati = 0;
        uint64_t previouskeyframetimest = 0;

        std::list<PositionTimestamp> _keyframes;  //文件快进位置
        bool hasFilepositionTimes = false;
        void push(double p4, double p5) {
            auto t = static_cast<uint32_t>(p5);
            auto ps = static_cast<size_t>(p4);
            _keyframes.push_back({t, ps});
        }
        void update() {
            if (!_keyframes.empty()) {
                hasFilepositionTimes = true;
                // cout << "解析元数据成功" << endl;
            }
        }

        void seek(int64_t s, int& _headLen) {
            for (auto& v : _keyframes) {
                if (v.timestamp > s) {
                    break;
                }
                _headLen = v.position;
                previouskeyframetimest = v.timestamp;
            }
            return;
        }

        bool getFilepositionTimes(AMFObject& obj) {
            std::string text;
            AMFObjectProperty* p0 = AMF_GetProp(&obj, NULL, 1);
            if (p0 && (p0->p_type == AMF_ECMA_ARRAY || p0->p_type == AMF_OBJECT)) {
                AMFObject* obj0 = &p0->p_vu.p_object;
                AMFObjectProperty* p1 = NULL;
                for (int i = 0; i < obj0->o_num; i++) {
                    p1 = AMF_GetProp(obj0, NULL, i);
                    text = std::string(p1->p_name.av_val, p1->p_name.av_len);
                    if (text == "metadatacreator") {
                        metadatacreator = std::string(p1->p_vu.p_aval.av_val, p1->p_vu.p_aval.av_len);
                    } else if (text == "hasKeyframes") {
                        hasKeyframes = p1->p_vu.p_number;
                    } else if (text == "hasVideo") {
                        hasVideo = p1->p_vu.p_number;
                    } else if (text == "hasAudio") {
                        hasAudio = p1->p_vu.p_number;
                    } else if (text == "hasMetadata") {
                        hasMetadata = p1->p_vu.p_number;
                    } else if (text == "canSeekToEnd") {
                        canSeekToEnd = p1->p_vu.p_number;
                    } else if (text == "duration") {
                        duration = p1->p_vu.p_number;
                    } else if (text == "datasize") {
                        datasize = p1->p_vu.p_number;
                    } else if (text == "videosize") {
                        videosize = p1->p_vu.p_number;
                    } else if (text == "framerate") {
                        framerate = p1->p_vu.p_number;
                    } else if (text == "videodatarate") {
                        videodatarate = p1->p_vu.p_number;
                    } else if (text == "videocodecid") {
                        videocodecid = p1->p_vu.p_number;
                    } else if (text == "width") {
                        width = p1->p_vu.p_number;
                    } else if (text == "height") {
                        height = p1->p_vu.p_number;
                    } else if (text == "audiosize") {
                        audiosize = p1->p_vu.p_number;
                    } else if (text == "audiodatarate") {
                        audiodatarate = p1->p_vu.p_number;
                    } else if (text == "audiocodecid") {
                        audiocodecid = p1->p_vu.p_number;
                    } else if (text == "audiosamplerate") {
                        audiosamplerate = p1->p_vu.p_number;
                    } else if (text == "audiosamplesize") {
                        audiosamplesize = p1->p_vu.p_number;
                    } else if (text == "stereo") {
                        stereo = p1->p_vu.p_number;
                    } else if (text == "filesize") {
                        filesize = p1->p_vu.p_number;
                    } else if (text == "lasttimestamp") {
                        lasttimestamp = p1->p_vu.p_number;
                    } else if (text == "lastkeyframetimest") {
                        lastkeyframetimest = p1->p_vu.p_number;
                    } else if (text == "lastkeyframelocati") {
                        lastkeyframelocati = p1->p_vu.p_number;
                    } else if (text == "keyframes") {
                        AMFObjectProperty* p2 = AMF_GetProp(&p1->p_vu.p_object, NULL, 0);  //
                        text = std::string(p2->p_name.av_val, p2->p_name.av_len);
                        AMFObjectProperty* p3 = AMF_GetProp(&p1->p_vu.p_object, NULL, 1);  //
                        if (p2 && (p2->p_type == AMF_STRICT_ARRAY || p2->p_type == AMF_OBJECT) && p3 && (p3->p_type == AMF_STRICT_ARRAY || p3->p_type == AMF_OBJECT) && p2->p_vu.p_object.o_num == p3->p_vu.p_object.o_num) {
                            for (int j = 0; j < p2->p_vu.p_object.o_num; j++) {
                                AMFObjectProperty* p4 = AMF_GetProp(&p2->p_vu.p_object, NULL, j);  //
                                AMFObjectProperty* p5 = AMF_GetProp(&p3->p_vu.p_object, NULL, j);  //
                                push(p4->p_vu.p_number, p5->p_vu.p_number * 1000.0);
                            }
                        }
                    }
                }
            }
            update();
            return true;
        }
    };
    PositionManager positionManager;

    bool find_1st_key_frame_ = false;  //是否是首次视频帧，保证首次视频是关键帧

    enum {
        kBufferSize = 256,
        kBodyBufferSize = 512 * 1024,
        kDataLength = 1024 * 1280
    };
    struct Span {
        const uint8_t* data;
        int size;
        const uint8_t* DataEnd() const {
            return data + size;
        }
    };

    struct Frame {
        uint8_t* data = nullptr;
        size_t data_length = 0;
        void WriteData(uint8_t* d, int l) {
            assert(data_length + l <= kDataLength);
            memcpy(data + data_length, d, l);
            data_length += l;
        }
        void WriteData(const Span& span) {
            assert(data_length + span.size <= kDataLength);
            memcpy(data + data_length, span.data, span.size);
            data_length += span.size;
        }
        void WriteH264Header() {
            uint8_t h264begin[] = {0x00, 0x00, 0x00, 0x01};
            WriteData(h264begin, 4);
        }

        Frame() {
            data = new uint8_t[kDataLength];
        }
        ~Frame() {
            if (data) {
                delete[] data;
            }
            reset();
        }

        uint8_t tagType = 0;       //帧类型
        int32_t timestamp = 0;     //时间戳
        uint32_t streamsId = 0;    //
        uint8_t* body = nullptr;   //帧数据
        uint32_t body_length = 0;  //帧数据长度

        const uint8_t* BodyEnd() const {
            return body + body_length;
        }

        void reset() {
            if (body != nullptr) {
                delete body;
                body = nullptr;
            }
        }
    };

    struct NaluHelper {
        // todo：NALU包长度字节数
        int naluPackageLenBits = -1;

        int CheckNalu(Frame& one_frame, uint8_t* dataTmp, int default_value) {
            // note：NALU
            int nowlen = 0;
            int nNauluLen = 0;
            // note：cts：0x000000，三个字节
            dataTmp += 5;
            while (nowlen < one_frame.body_length - 5) {
                switch (naluPackageLenBits) {
                    case 4:
                        nNauluLen = LEN4_((&(dataTmp[0])));
                        break;
                    case 3:
                        nNauluLen = LEN3_((&(dataTmp[0])));
                        break;
                    case 2:
                        nNauluLen = LEN2_((&(dataTmp[0])));
                        break;
                    case 1:
                        nNauluLen = LEN1_((&(dataTmp[0])));
                        break;
                    default:
                        return kDontCare;
                }
                dataTmp += naluPackageLenBits;
                nowlen += (naluPackageLenBits + nNauluLen);

                one_frame.WriteH264Header();

                if (dataTmp + nNauluLen > one_frame.BodyEnd()) {
                    return kDontCare;
                }

                one_frame.WriteData(dataTmp, nNauluLen);
                dataTmp += nNauluLen;
            }
            return default_value;
        }
    };
    NaluHelper nalu_helper;

    Frame frame_;
    std::ifstream fp_in;
    std::mutex readMtx_;
    // note: 当前播放的时间戳
    uint32_t currentTime = 0;
    std::mutex pauseMtx_;
    bool hasPause = false;
    bool stop = false;
    // note: 全局播放时间，倍速播放使用
    uint64_t global_time = 0;
    int kStepTime = 40;

    void updateThread();
    void startParse() {
        parse = std::thread(&FlvPlayer::updateThread, this);
    }
    std::thread parse;

    void stopParse();

    int getH264Data(Frame& one_frame);

    void seekTo(int64_t time);

    void pause();

    void play();
};