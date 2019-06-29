//todo: 仅支持audio：speex/video：h264的flv数据，16000 1 s16

#include <fstream>
#include <iostream>
#include <list>
#include <thread>

#include <SDL2/SDL.h>
#include <librtmp/amf.h>
#include <speex/speex.h>

extern "C" {
#include <speex/speex_types.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}

#define USING_SDL
#define LOG

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
public:
    Frame frame_;

    int getH264Data(Frame& one_frame) {
        uint8_t *dataTmp = one_frame.body;

        Span sps;
        Span pps;

        if (dataTmp[0] == 0x17) {
            // note：onMetadata：sps/pps
            if (dataTmp[1] == 0x00) {  //AVCDecoderConfigurationRecord
                // note：cts 0x000000，三个字节
                nalu_helper.naluPackageLenBits = (dataTmp[9] & 0x03) + 1;

                int numOfSequenceParameterSets = dataTmp[10] & 0x1F;  //sps数量
                dataTmp += 11;

                for (int i = 0; i < numOfSequenceParameterSets; i++) {  //(data[num]&0x000000FF)<<24|(data[num+1]&0x000000FF)<<16|(data[num+2]&0x000000FF)<<8|data[num+3]&0x000000FF;
                    // note：pps length
                    int sequenceParameterSetLength = LEN2_((&(dataTmp[0])));
                    sps.data = dataTmp + 2;
                    sps.size = sequenceParameterSetLength;

                    dataTmp += 2;
                    dataTmp += sequenceParameterSetLength;
                }
                int numOfPictureParameterSets = LEN1_((&(dataTmp[0])));
                dataTmp += 1;
                for (int i = 0; i < numOfPictureParameterSets; i++) {
                    int pictureParameterSetLength = LEN2_((&(dataTmp[0])));
                    pps.data = dataTmp + 2;
                    pps.size = pictureParameterSetLength;
                    dataTmp += 2;
                    dataTmp += pictureParameterSetLength;
                }
                one_frame.data_length = 0;
                one_frame.WriteH264Header();

                if (sps.DataEnd() > one_frame.BodyEnd()) {
                    return kDontCare;
                }
                one_frame.WriteData(sps);
                one_frame.WriteH264Header();

                if (pps.DataEnd() > one_frame.BodyEnd()) {
                    return kDontCare;
                }
                one_frame.WriteData(pps);
                return kPpsSps;
            } else if (dataTmp[1] == 0x01) {
                return nalu_helper.CheckNalu(one_frame, dataTmp, kKeyVideo);
            }
        } else if (dataTmp[0] == 0x27) {
            if (!find_1st_key_frame_)
                return kFullVideo;
            if (dataTmp[1] == 0x01) {
                return nalu_helper.CheckNalu(one_frame, dataTmp, kFullVideo);
            }
        }
        return kDontCare;
    }
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

#define REFRESH_EVENT  (SDL_USEREVENT + 1)
#define BREAK_EVENT  (SDL_USEREVENT + 2)

struct SDL {
    SDL_Window *screen = nullptr;
    SDL_Renderer* sdlRenderer = nullptr;
    SDL_Texture* sdlTexture = nullptr;
    int screen_w = 0;
    int screen_h = 0;
    static bool thread_exit;
    SDL_Rect sdlRect;
    bool sdl_init() {
        if (SDL_Init(SDL_INIT_AUDIO)) {
            return false;
        }
        screen = SDL_CreateWindow("Simplest Video Play SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  screen_w, screen_h,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
        if (!screen) {
            return false;
        }
        return true;
    }

    void setVideo(int pixel_w, int pixel_h) {
        sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
        sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, pixel_w, pixel_h);
    }

    static int refresh_video(void *opaque) {
        thread_exit = false;
        while (!thread_exit) {
            SDL_Event event;
            event.type = REFRESH_EVENT;
            SDL_PushEvent(&event);
            SDL_Delay(40);
        }
        thread_exit = true;
        SDL_Event event;
        event.type = BREAK_EVENT;
        SDL_PushEvent(&event);
    }

    void startVideoSuffer() {
        SDL_Thread *refresh_thread = SDL_CreateThread(refresh_video, nullptr, nullptr);
    }

    void startEvent() {
        SDL_Event event;
        while (1) {
            SDL_WaitEvent(&event);
            if (event.type == REFRESH_EVENT) {
            } else if (event.type==SDL_WINDOWEVENT) {

            } else if (event.type==SDL_QUIT) {
                thread_exit = true;
            } else if (event.type==BREAK_EVENT) {
                break;
            }
        }
        SDL_Quit();
        return;
    }

    void startVideoEvent() {

    }
};

class H264Decode {
    AVCodecContext* codecCtx_ = nullptr;
    AVCodec* videoCodec = nullptr;
public:
    bool success = false;
    AVFrame *frame = nullptr;
    H264Decode() {
        frame = av_frame_alloc();
        avcodec_register_all();
        av_log_set_level(AV_LOG_QUIET);
        videoCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
        codecCtx_ = avcodec_alloc_context3(videoCodec);
        avcodec_open2(codecCtx_, videoCodec, nullptr);
    }

    ~H264Decode() {
        if (frame) {
            av_frame_free(&frame);
        }
        if (codecCtx_) {
            avcodec_close(codecCtx_);
            avcodec_free_context(&codecCtx_);
        }
    }

    bool Decode(uint8_t* buf, uint32_t size, int& width, int& height, uint8_t* yuv) {
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = buf;
        pkt.size = size;

        int result = avcodec_send_packet(codecCtx_, &pkt);
        if (result < 0) {
            return false;
        }
        while (result == 0) {
            result = avcodec_receive_frame(codecCtx_, frame);
            if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
                break;
            }
            if (result < 0) {
                return false;
            }
            width = frame->width;
            height = frame->height;
            int size = av_image_get_buffer_size((AVPixelFormat)frame->format, width, height, 1);
            assert(size > 0);
            int ret = av_image_copy_to_buffer(yuv, size, (const uint8_t* const *)frame->data, (const int *)frame->linesize, (AVPixelFormat)frame->format, width, height, 1);
            assert(ret > 0);
            success = true;
        }
        return true;
    }
};

int main() {
    int index = 0;
    FlvPlayer flvPlayer;
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

    H264Decode decode;
    uint8_t* yuv = new uint8_t[1024 * 1024];
    int width = 0;
    int height = 0;

    std::ifstream fp_in;

#if !defined(USING_SDL)
    std::ofstream fp_out_audio;
    fp_out_audio.open("./haha.pcm", std::ios::app | std::ios::out | std::ios::binary);
    std::ofstream fp_out_video;
    fp_out_video.open("./haha.yuv", std::ios::app | std::ios::out | std::ios::binary);
#else
    SDL sdl;
//    sdl.sdl_init();
#endif
//    fp_in.open("/Users/guochao/Downloads/out-audio-0aee7de51c5b44b8a5f345b146f83809-vs_f_1561097554498_t_1561097988738.flv", ios::in);
//   todo: sing.flv 图片 + speex test.flv h264 + speex
//    fp_in.open("/Users/guochao/DBY_code/ff_test/sing.flv", std::ios::in);
    fp_in.open("/Users/guochao/DBY_code/ff_test/test.flv", std::ios::in);

    Header header;
    fp_in.read((char*)header.header_data, 9);
    Body buffer;

    while (!fp_in.eof()) {
        fp_in.read((char*)buffer.body_data, 4);
        // todo: 返回上一次read的字节数
        if (fp_in.gcount() == 0) {
            break;
        }
        fp_in.read((char*)buffer.body_data, 11);
        if (fp_in.gcount() == 0) {
            break;
        }
        flvPlayer.frame_.tagType = buffer.body_data[0];
        flvPlayer.frame_.body_length = LEN3_((&buffer.body_data[1]));
        flvPlayer.frame_.timestamp = TIME4_((&buffer.body_data[4]));
        flvPlayer.frame_.body = new uint8_t[flvPlayer.frame_.body_length];

        fp_in.read((char*)flvPlayer.frame_.body, flvPlayer.frame_.body_length);

        if (flvPlayer.frame_.tagType == 8) {
#if defined(LOG)
            std::cout << "音频数据 : " << flvPlayer.frame_.body_length << " index = " << index << std::endl;
#endif
            speex_bits_reset(&bits);
            speex_bits_read_from(&bits, (char *)flvPlayer.frame_.body + 1, flvPlayer.frame_.body_length - 1);
            int ret = speex_decode_int(dec_state, &bits, output_frame);
            while (ret == 0) {
#if !defined(USING_SDL)
                fp_out_audio.write((char *)output_frame, frame_size * 2);
#else
#endif
                ret = speex_decode_int(dec_state, &bits, output_frame);
            }
        } else if (flvPlayer.frame_.tagType == 9) {
#if defined(LOG)
            std::cout << "视频数据 : " << flvPlayer.frame_.body_length << " index = " << index << std::endl;
#endif
            int ret = flvPlayer.getH264Data(flvPlayer.frame_);

            if (ret == kPpsSps) {
                flvPlayer.find_1st_key_frame_ = true;
            } else if (ret == kKeyVideo) {
                decode.Decode(flvPlayer.frame_.data, flvPlayer.frame_.data_length, width, height, yuv);
                flvPlayer.frame_.data_length = 0;
            } else if (ret == kFullVideo) {
                if (flvPlayer.find_1st_key_frame_) {
                    decode.Decode(flvPlayer.frame_.data, flvPlayer.frame_.data_length, width, height, yuv);
                    flvPlayer.frame_.data_length = 0;
                }
            }
            if (decode.success) {
#if !defined(USING_SDL)
                fp_out_video.write((char *)yuv, width * height * 1.5);
#else
                sdl.sdlRect.x = 0;
                sdl.sdlRect.y = 0;
                sdl.sdlRect.w = width;
                sdl.sdlRect.h = height;
#endif
#if 0
                char *buf = new char[decode.frame->width * decode.frame->height * 3 / 2];
                int a = 0;
                for (int i = 0; i < decode.frame->height; i++) {
                    memcpy(buf + a, decode.frame->data[0] + i * decode.frame->linesize[0], decode.frame->width);
                    a += decode.frame->width;
                }
                for (int i = 0; i < decode.frame->height / 2; i++) {
                    memcpy(buf + a, decode.frame->data[1] + i * decode.frame->linesize[1], decode.frame->width / 2);
                    a += decode.frame->width / 2;
                }
                for (int i = 0; i < decode.frame->height / 2; i++) {
                    memcpy(buf + a, decode.frame->data[2] + i * decode.frame->linesize[2], decode.frame->width / 2);
                    a += decode.frame->width / 2;
                }
                fp_out_video.write(buf, decode.frame->width * decode.frame->height * 1.5);
                delete[] buf;
                buf = NULL;
                decode.success = false;
#endif
            }
//            fp_in.seekg(flvPlayer.frame_.body_length, fp_in.cur);
        } else if (flvPlayer.frame_.tagType == 18) {
            std::cout << "flv元数据 : " << flvPlayer.frame_.body_length << " index = " << index << std::endl;
            AMFObject obj;
            int ret = AMF_Decode(&obj, (char*)flvPlayer.frame_.body, flvPlayer.frame_.body_length, FALSE);
            if (ret < 0) {
                std::cout << "AMF_Decode failed" << std::endl;
                delete [] flvPlayer.frame_.body;
                break;
            }
            flvPlayer.positionManager.getFilepositionTimes(obj);
            AMF_Reset(&obj);
        } else {
            std::cout << "unknow : " << flvPlayer.frame_.body_length << " type : " << flvPlayer.frame_.tagType << std::endl;
            fp_in.seekg(flvPlayer.frame_.body_length, fp_in.cur);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        index++;
        flvPlayer.frame_.reset();
    }
    std::cout << "flv读取结束" << std::endl;

    delete[] output_frame;
    delete[] yuv;

#if !defined(USING_SDL)
    fp_out_audio.close();
    fp_out_video.close();
#endif

    return 0;
}