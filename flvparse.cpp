#include <fstream>
#include <iostream>
#include <list>
#include <thread>

#include <speex.h>
#include <speex_types.h>

#include <SDL2/SDL.h>

extern "C" {
#include <amf.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}

using namespace std;

// #define USING_SDL
// #define AUIDO_DECODE
// #define VIDEO_DECODE
// #define LOG

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

    int seek(int64_t s, int _headLen) {
        int seekTmp = _headLen;
        for (const auto& v : _keyframes) {
            if (v.timestamp > s) {
                break;
            }
            seekTmp = v.position;
        }
        return seekTmp;
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

class Msg {
public:
    uint8_t tagType = 0;       //帧类型
    uint32_t body_length = 0;  //帧数据长度
    uint8_t* body = nullptr;   //帧数据
    int32_t timestamp = 0;     //时间戳
    int audio_frame = 0;       //音频帧
    int video_frame = 0;       //视频帧
    int scrpit_frame = 0;      //元数据
};

Uint32 audio_len;
Uint8* audio_chunk;
Uint8* audio_pos;

static void sdl_audio_callback(void* opaque, Uint8* stream, int len) {
    if (audio_len == 0) {
        return;
    }
    len = (audio_len > len ? len : audio_len);
    SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
    audio_pos += len;
    audio_len -= len;
}

struct SDL {
    bool sdl_init() {
        if (SDL_Init(SDL_INIT_AUDIO)) {
            return false;
        }
        wanted_spec.freq = 16000;
        wanted_spec.format = AUDIO_S16SYS;
        wanted_spec.channels = 1;
        wanted_spec.silence = 0;
        wanted_spec.samples = 320;
        wanted_spec.callback = sdl_audio_callback;
        wanted_spec.userdata = nullptr;

        if (SDL_OpenAudio(&wanted_spec, NULL) < 0) {
            printf("can't open audio.\n");
            return -1;
        }
        return true;
    }
    SDL_AudioSpec wanted_spec;
};

class H264Decode {
    AVCodecContext* codecCtx_ = nullptr;
    AVCodec* videoCodec = nullptr;

public:
    H264Decode() {
        avcodec_register_all();
        av_log_set_level(AV_LOG_QUIET);
        videoCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
        codecCtx_ = avcodec_alloc_context3(videoCodec);
        avcodec_open2(codecCtx_, videoCodec, nullptr);
    }
    bool Decode(uint8_t* buf, uint32_t size, uint8_t* yuv, uint32_t& yuv_size) {
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = buf;
        pkt.size = size;

        AVFrame* frame = nullptr;
        frame = av_frame_alloc();

        int result = avcodec_send_packet(codecCtx_, &pkt);
        if (result < 0) {
            return false;
        }
        while (result > 0) {
            result = avcodec_receive_frame(codecCtx_, frame);
            if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
                break;
            }
            if (result < 0) {
                return false;
            }
            yuv_size = av_image_get_buffer_size((AVPixelFormat)frame->format, frame->width, frame->height, 1);
            memcpy(yuv, frame->data[0], yuv_size);
        }
        return true;
    }
};

int main() {
    int index = 0;
#if defined(AUIDO_DECODE)
    SpeexBits bits;
    void* dec_state = nullptr;
    int frame_size = 0;               //每帧的大小
    int16_t* output_frame = nullptr;  //输出的大小
    speex_bits_init(&bits);
    dec_state = speex_decoder_init(speex_lib_get_mode(SPEEX_MODEID_WB));
    speex_decoder_ctl(dec_state, SPEEX_GET_FRAME_SIZE, &frame_size);

    int speex_enh = 1;
    speex_decoder_ctl(dec_state, SPEEX_SET_ENH, &speex_enh);
    output_frame = new int16_t[frame_size];
#endif

#if defined(VIDEO_DECODE)
    H264Decode decode;
    uint8_t* yuv = (uint8_t*)malloc(1024 * 1024);
    uint32_t yuv_size = 0;
#endif
    ifstream fp_in;

#if !defined(USING_SDL)
    ofstream fp_out_audio;
    fp_out_audio.open("./haha.pcm", ios::app | ios::out | ios::binary);
    ofstream fp_out_video;
    fp_out_video.open("./haha.yuv", ios::app | ios::out | ios::binary);
#else
    SDL sdl;
    sdl.sdl_init();
    SDL_PauseAudio(0);
#endif
    fp_in.open("/Users/guochao/Downloads/out-audio-0aee7de51c5b44b8a5f345b146f83809-vs_f_1561097554498_t_1561097988738.flv", ios::in);

    Header header;
    fp_in.read((char*)header.header_data, 9);
    Body body;
    Msg msg;
    PositionManager positionManager;

    while (!fp_in.eof()) {
        fp_in.read((char*)body.body_data, 4);
        // todo: 返回上一次read的字节数
        if (fp_in.gcount() == 0) {
            break;
        }
        fp_in.read((char*)body.body_data, 11);
        if (fp_in.gcount() == 0) {
            break;
        }
        msg.tagType = body.body_data[0];
        msg.body_length = LEN3_((&body.body_data[1]));
        msg.timestamp = TIME4_((&body.body_data[4]));
        msg.body = new uint8_t[msg.body_length];

        if (msg.tagType == 8) {
            msg.audio_frame++;
#if defined(LOG)
            cout << "音频数据 : " << msg.body_length << " index = " << index << endl;
#endif
            fp_in.read((char*)msg.body, msg.body_length);
#if defined(AUIDO_DECODE)
            speex_bits_reset(&bits);
            speex_bits_read_from(&bits, (char*)msg.body + 1, msg.body_length - 1);
            int ret = speex_decode_int(dec_state, &bits, output_frame);
            while (ret == 0) {
#if !defined(USING_SDL)
                fp_out_audio.write((char*)output_frame, frame_size * 2);
#else
                audio_chunk = (Uint8*)output_frame;
                audio_len = frame_size * 2;
                audio_pos = audio_chunk;
                while (audio_len > 0) {
                    SDL_Delay(1);
                }
#endif
                ret = speex_decode_int(dec_state, &bits, output_frame);
            }
#endif
        } else if (msg.tagType == 9) {
            msg.video_frame++;
#if defined(LOG)
            cout << "视频数据 : " << msg.body_length << " index = " << index << endl;
#endif
            if (msg.timestamp == 233634 || msg.timestamp == 2150946) {
                cout << "视频数据 : " << msg.timestamp << endl;
            }
#if defined(VIDEO_DECODE)
            decode.Decode(msg.body, msg.body_length, yuv, yuv_size);
#if !defined(USING_SDL)
//            fp_out_video.write((char *)yuv, yuv_size);
#endif
#endif
            fp_in.seekg(msg.body_length, fp_in.cur);
        } else if (msg.tagType == 18) {
            msg.scrpit_frame++;
            cout << "flv元数据 : " << msg.body_length << endl;
            fp_in.read((char*)msg.body, msg.body_length);
            AMFObject obj;
            int ret = AMF_Decode(&obj, (char*)msg.body, msg.body_length, FALSE);
            if (ret < 0) {
                std::cout << "AMF_Decode failed" << std::endl;
                delete [] msg.body;
                break;
            }
            positionManager.getFilepositionTimes(obj);
            AMF_Reset(&obj);
        } else {
            cout << "unknow : " << msg.body_length << " type : " << msg.tagType << endl;
            fp_in.seekg(msg.body_length, fp_in.cur);
            this_thread::sleep_for(chrono::milliseconds(1000));
        }
        index++;
        delete[] msg.body;
    }
    std::cout << "flv读取结束" << std::endl;

#if !defined(USING_SDL)
    fp_out_audio.close();
    fp_out_video.close();
#endif

    return 0;
}