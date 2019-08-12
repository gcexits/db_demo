#include <iostream>
#include <fstream>

extern "C" {
#include <libavformat/avformat.h>
}

#include "HttpFile.h"
#include "SDLPlayer.h"
#include "H264Decoder.h"

#include "../flv_parse/hlring/RingBuffer.h"
#include "../flv_parse/hlring/rbuf.h"

class Demuxer {
public:
    enum class ReadStatus {
        Video,
        Audio,
        Subtitle,
        Error
    };

private:
    bool opened_ = false;
    int videoindex = -1;
    int audioindex = -1;
    int scan_all_pmts_set = 0;

public:
    Demuxer() {
        pkt = av_packet_alloc();
    }
    ~Demuxer() {
        Close();
        av_packet_free(&pkt);
    }
    struct AVFormatContext* ifmt_ctx = nullptr;
    AVBitStreamFilterContext* h264bsfc = nullptr;
    bool need_free_ = true;
    struct AVPacket* pkt = nullptr;
    bool Opened() {
        return opened_;
    }
    void Close() {
        if (Opened()) {
            if (need_free_ && ifmt_ctx) {
                avformat_close_input(&ifmt_ctx);
                avformat_free_context(ifmt_ctx);
                ifmt_ctx = nullptr;
            }
            scan_all_pmts_set = 0;
            videoindex = audioindex = -1;
            opened_ = false;
        }
    }

    bool Open(const std::string& inUrl) {}
    bool Open(void* param) {
        ifmt_ctx = (struct AVFormatContext *)param;
        need_free_ = false;
        int ret = avformat_find_stream_info(ifmt_ctx, NULL);
        if (ret < 0) {
            printf("Could not find stream information\n");
            return false;
        }
        videoindex = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        audioindex = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        opened_ = true;
        return true;
    }

    // todo: 添加sps pps头
    bool addSpsPps(AVPacket *pkt, AVCodecParameters *codecpar) {
        const AVBitStreamFilter *avBitStreamFilter = nullptr;
        AVBSFContext *absCtx = NULL;

        avBitStreamFilter = av_bsf_get_by_name("h264_mp4toannexb");

        av_bsf_alloc(avBitStreamFilter, &absCtx);

        avcodec_parameters_copy(absCtx->par_in, codecpar);

        av_bsf_init(absCtx);

        if(av_bsf_send_packet(absCtx, pkt) != 0){
            return false;
        }

        while(av_bsf_receive_packet(absCtx, pkt) == 0) {
            continue;
        }

        av_bsf_free(&absCtx);
        absCtx = NULL;
        return true;
    }

    // todo: 视频:0, 音频:1, 字幕:2, 失败:-1
    ReadStatus ReadFrame(std::ofstream &fp) {
        if (av_read_frame(ifmt_ctx, pkt) < 0) {
            //av_seek_frame(ifmt_ctx, pkt->stream_index, 0, AVSEEK_FLAG_BACKWARD);
            return ReadStatus::Error;
        }
        if (pkt->stream_index == videoindex) {
            static H264Decode video_decode(ifmt_ctx->streams[videoindex]->codec);
//            addSpsPps(pkt, ifmt_ctx->streams[videoindex]->codecpar);
//            fp.write((char *)pkt->data, pkt->size);
            video_decode.Decode(pkt->data, pkt->size);
            return ReadStatus::Video;
        } else if (pkt->stream_index == audioindex) {
            return ReadStatus::Audio;
        } else {
            return ReadStatus::Subtitle;
        }
    }

    bool isKeyFrame() {
        return pkt->flags & AV_PKT_FLAG_KEY;
    }

    AVCodecParameters* audioCodecPar() {
        return ifmt_ctx->streams[audioindex]->codecpar;
    }
};

class IOBufferContext {
    int buffer_size = 1024 * 4;
    AVIOContext* avio_ctx = nullptr;
    AVFormatContext* fmt_ctx = nullptr;
    bool opened_ = false;

    RingBuffer ringBuffer;
    const size_t RingLength;
    std::ofstream fp_douyin;

    int read_packet(uint8_t* buffer, int length) {
        while (ringBuffer.size() < length && !io_sync.exit) {
            //WriteDebugLog("read %d, return %d", length, ringBuffer.size());
            std::unique_lock<std::mutex> lk(io_sync.m);
            io_sync.cv.wait(lk);
        }

        if (io_sync.exit) {
            return AVERROR_EOF;
        }
        return ringBuffer.read(buffer, length);
    }

public:
    struct {
        mutable std::mutex m;
        mutable std::condition_variable cv;
        //bool ready = false;
        bool exit = false;
    } io_sync;

    int OpenInput() {
        size_t max_length_ = 1024 * 320;
        size_t size = std::min(RingLength, max_length_);
        while (ringBuffer.size() < size / 2 && !io_sync.exit) {
            std::unique_lock<std::mutex> lk(io_sync.m);
            io_sync.cv.wait(lk);
        }

        if (io_sync.exit) {
            // IOBufferContext 退出
            return AVERROR_EXIT;
        }

        uint8_t *buffer = (uint8_t *)av_malloc(buffer_size);
        if (!buffer) {
            return AVERROR(ENOMEM);
        }
        avio_ctx = avio_alloc_context(buffer, buffer_size, 0, this, &IOBufferContext::ReadPacket, nullptr, nullptr);
        if (!avio_ctx) {
            return AVERROR(ENOMEM);
        }

        fmt_ctx = avformat_alloc_context();
        if (!fmt_ctx) {
            return AVERROR(ENOMEM);
        }
        fmt_ctx->pb = avio_ctx;
        fmt_ctx->flags = AVFMT_FLAG_CUSTOM_IO;

        int ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
        opened_ = ret == 0;
        return ret;
    }
    explicit IOBufferContext(int length) : RingLength(length), ringBuffer("IOBufferContext", length) {}
    ~IOBufferContext() {
        io_sync.exit = true;
        {
            std::unique_lock<std::mutex> lk(io_sync.m);
            io_sync.cv.notify_all();
        }

        if (avio_ctx) {
            av_freep(&avio_ctx->buffer);  // avio_ctx_buffer
            av_freep(&avio_ctx);
        }

        if (fmt_ctx) {
            avformat_close_input(&fmt_ctx);
            fmt_ctx = nullptr;
        }
    }

    int FillBuffer(uint8_t* buffer, int length) {
        int len = ringBuffer.write(buffer, length);
        std::unique_lock<std::mutex> lk(io_sync.m);
        io_sync.cv.notify_all();
        return len;
    }

    static int ReadPacket(void* opaque, uint8_t* buffer, int length) {
        IOBufferContext* playback = (IOBufferContext*)opaque;
        return playback->read_packet(buffer, length);
    }

    void* getFormatContext(bool* status) {
        if (status) {
            *status = opened_;
        }
        return fmt_ctx;
    }
};

void RegisterPlayer() {
    using namespace std::placeholders;

    SDLPlayer *player = SDLPlayer::getPlayer();
    AVRegister::setinitVideoPlayer(std::bind(&SDLPlayer::openVideo, player, _1, _2));
    AVRegister::setinitPcmPlayer(std::bind(&SDLPlayer::openAudio, player, _1, _2));
}

int main() {
    RegisterPlayer();
    int buffer_size = 1024 * 4;
    duobei::HttpFile httpFile;
    int ret = httpFile.Open("http://v3-dy.ixigua.com/6b0e04991e1a021b16a1fc19dcdf1f3e/5d50e77d/video/m/220436e769ae59341d6acb40b42515d580611632036b0000538449e668bf/?rc=amRscTh0NnF4bzMzO2kzM0ApdSk1Njc1MzM4MzM4NDQ0MzQ1bzQ6Z2UzZDQ1ZGVnZDw2ZDdAaUBoNnYpQGczdilAZjM7NEBgZXIwLjFhNTJfLS1jLS9zczppPzQwNi0vMy4uLS8uLzU2LTojXmAxLV5hYTU2MS8xLTE0YGEjbyM6YS1vIzpgLW8jLS8uXg%3D%3D");
//    int ret = httpFile.Open("https://playback2.duobeiyun.com/jz0caeb823fb764ad9abc4a39330851fe8/streams/out-video-jz04e17fa4dc904e5c91f75bf92bc31f55_f_1565175600703_t_1565179641893.flv");
    if (ret != duobei::FILEOK) {
        std::cout << "url error" << std::endl;
        return -1;
    }
    uint8_t *buffer = new uint8_t[buffer_size + 1];

    IOBufferContext ioBufferContext(1024 * 3200);
    bool ready = false;
    std::thread readthread;
    Demuxer demuxer;

    // todo: mp4解封装 保存h264流有问题，需要av_bitstream_filter_init给h264裸流添加sps pps
    std::ofstream fp;
    if (!fp.is_open()) {
        fp.open("douyin.h264", std::ios::out | std::ios::binary | std::ios::ate);
    }
    while (httpFile.Read(buffer, buffer_size, buffer_size) != duobei::FILEEND) {
        ioBufferContext.FillBuffer(buffer, buffer_size);
        if (!ready) {
            readthread = std::thread([&ioBufferContext, &demuxer, &fp]{
                bool status = false;
                void *param = nullptr;
                if (!status) {
                    do {
                        ioBufferContext.OpenInput();
                        param = (AVFormatContext *)ioBufferContext.getFormatContext(&status);
                    } while (!status);
                    demuxer.Open(param);
                }
                while (1) {
                    if (demuxer.ReadFrame(fp) == Demuxer::ReadStatus::Error) {
                        break;
                    }
                }
            });
        }
        ready = true;
    }
    SDLPlayer::getPlayer()->EventLoop();
    ioBufferContext.io_sync.exit = true;
    if (readthread.joinable()) {
        readthread.join();
    }
    httpFile.Close();
    fp.close();
    std::cout << "curl file end" << std::endl;
    return 0;
}