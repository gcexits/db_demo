extern "C" {
#include <libavformat/avformat.h>
}

#include "AACDecoder.h"
#include "H264Decoder.h"
#include "HttpFile.h"
#include "SDLPlayer.h"

#include "../flv_parse/hlring/RingBuffer.h"
#include "../flv_parse/hlring/rbuf.h"

class Demuxer {
public:
    enum class ReadStatus {
        Video,
        Audio,
        Subtitle,
        Error,
        EndOff
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
    H264Decode video_decode;
    AACDecode audio_decode;
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
        ifmt_ctx = (struct AVFormatContext*)param;
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
    bool addSpsPps(AVPacket* pkt, AVCodecParameters* codecpar) {
        const AVBitStreamFilter* avBitStreamFilter = nullptr;
        AVBSFContext* absCtx = NULL;

        avBitStreamFilter = av_bsf_get_by_name("h264_mp4toannexb");

        av_bsf_alloc(avBitStreamFilter, &absCtx);

        avcodec_parameters_copy(absCtx->par_in, codecpar);

        av_bsf_init(absCtx);

        if (av_bsf_send_packet(absCtx, pkt) != 0) {
            return false;
        }

        while (av_bsf_receive_packet(absCtx, pkt) == 0) {
            continue;
        }

        av_bsf_free(&absCtx);
        absCtx = NULL;
        return true;
    }

    // todo: 视频:0, 音频:1, 字幕:2, 失败:-1
    ReadStatus ReadFrame() {
        int ret = av_read_frame(ifmt_ctx, pkt);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                return ReadStatus::EndOff;
            }
            //av_seek_frame(ifmt_ctx, pkt->stream_index, 0, AVSEEK_FLAG_BACKWARD);
            return ReadStatus::Error;
        }
        if (pkt->stream_index == videoindex) {
            video_decode.OpenDecode(ifmt_ctx->streams[videoindex]->codecpar);
            // todo: 给h264裸流添加sps pps
            // addSpsPps(pkt, ifmt_ctx->streams[videoindex]->codecpar);
            video_decode.Decode(pkt->data, pkt->size);
            return ReadStatus::Video;
        } else if (pkt->stream_index == audioindex) {
            int buffer_size = 0;
            audio_decode.OpenDecode(ifmt_ctx->streams[audioindex]->codecpar);
            audio_decode.Decode(pkt);
            auto that = static_cast<AudioChannel*>(audio_decode.playInternal.handle);
            while (that->buffer_.size() > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
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
public:
    int buffer_size = 1024 * 4;
    AVIOContext* avio_ctx = nullptr;
    AVFormatContext* fmt_ctx = nullptr;
    bool opened_ = false;

    RingBuffer ringBuffer;
    const size_t RingLength;
    std::ofstream fp_douyin;
    std::mutex ringBufferMtx;

    int read_packet(uint8_t* buffer, int length) {
        int count = 100;
        while (ringBuffer.size() < length && !io_sync.exit) {
            if (--count == 0) {
                break;
            }
            //            std::cout << "line[" << __LINE__ << "] file[" << __FILE__ << "] : ringBuffer.size() " << ringBuffer.size() << ", read " << length << std::endl;
            std::unique_lock<std::mutex> lk(io_sync.m);
            io_sync.cv.wait(lk);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        if (io_sync.exit) {
            return AVERROR_EOF;
        }
        std::lock_guard<std::mutex> lck(ringBufferMtx);
        return ringBuffer.read(buffer, length);
    }

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

        uint8_t* buffer = (uint8_t*)av_malloc(buffer_size);
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
        AVDictionary *opts = nullptr;
        av_dict_set(&opts, "timeout", "6000", 0);

        int ret = avformat_open_input(&fmt_ctx, nullptr, nullptr, &opts);
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
        int len = 0;
        if (length != 0 && buffer) {
            std::lock_guard<std::mutex> lck(ringBufferMtx);
            len = ringBuffer.write(buffer, length);
        }
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

    SDLPlayer* player = SDLPlayer::getPlayer();
    AVRegister::setinitVideoPlayer(std::bind(&SDLPlayer::openVideo, player, _1, _2));
    AVRegister::setinitPcmPlayer(std::bind(&SDLPlayer::openAudio, player, _1, _2));
}

//#define SRCFILE

int main(int argc, char* argv[]) {
    RegisterPlayer();
    int buffer_size = 1024 * 320;
    int hasRead_size = 0;
    duobei::HttpFile httpFile;
    int ret = httpFile.Open("http://v3-dy-z.ixigua.com/579dcc25546a9733bb44c5a483cac124/5d5517c0/video/m/2202d1d61ee8d604306a5832a59e79517b911630f0c70000a17255d93705/?rc=MzZmczdmbThkbzMzM2kzM0ApcHpAbzc3OzkzNjozNDY7ODg7PDNAKWg5aTRpaDM7NDc1NTg3ODNnKXUpQGczdSlAZjN1KTU0ZGMxb2Fpc29mMF8tLTItMHNzYmJebyM0MzEzNi0uLS0wLy4uLS4vaWMwLmBhXl41MTIyNGBfYV46YzpiMHAjOmEtcCM6YDUuOg%3D%3D");
//    int ret = httpFile.Open("http://vodkgeyttp8.vod.126.net/cloudmusic/MDAwICAhITMwOTAwMDAwOA==/mv/304279/88f4918de91e55cc1a9889191f553ff1.mp4?wsSecret=343666b79ca4450d23c11299ce339c65&wsTime=1565775156");
//    std::string url = "https://www.youtube.com/watch\\?v\\=L6joGUdc6y4";
//    std::cout << url << std::endl;
//    int ret = httpFile.Open("https://www.youtube.com/watch\\?v\\=L6joGUdc6y4");
    // todo: 目前ffmpeg没支持解封装 不晓得为什么
//    int ret = httpFile.Open("https://www.sample-videos.com/video123/mp4/720/big_buck_bunny_720p_30mb.mp4");
    if (ret != duobei::FILEOK) {
        std::cout << "url error" << std::endl;
        return -1;
    }
    uint8_t* buffer = new uint8_t[buffer_size + 1];

    IOBufferContext ioBufferContext(0);
    bool ready = false;
    bool finish = false;
    std::thread readthread;
    Demuxer demuxer;

    // todo: mp4解封装 保存h264流有问题，需要av_bitstream_filter_init给h264裸流添加sps pps
#if defined(SRCFILE)
    std::ofstream fp;
    if (!fp.is_open()) {
        fp.open("douyin.mp4", std::ios::out | std::ios::binary | std::ios::ate);
    }
#endif
    while (1) {
        int ret = httpFile.Read(buffer, buffer_size, buffer_size, hasRead_size);
#if defined(SRCFILE)
        fp.write((char*)buffer, hasRead_size);
#endif
        ioBufferContext.FillBuffer(buffer, hasRead_size);
        if (ret == duobei::FILEEND) {
            break;
        }
        if (!ready) {
            readthread = std::thread([&ioBufferContext, &demuxer, &finish] {
                bool status = false;
                void* param = nullptr;
                if (!status) {
                    do {
                        ioBufferContext.OpenInput();
                        param = (AVFormatContext*)ioBufferContext.getFormatContext(&status);
                    } while (!status);
                    demuxer.Open(param);
                }
                while (1) {
                    if (demuxer.ReadFrame() == Demuxer::ReadStatus::EndOff) {
                        finish = true;
                        break;
                    }
                }
            });
        }
        ready = true;
    }
    while (!finish) {
        ioBufferContext.FillBuffer(nullptr, 0);
    }
    ioBufferContext.io_sync.exit = true;
    SDLPlayer::getPlayer()->EventLoop();
    if (readthread.joinable()) {
        readthread.join();
    }
    httpFile.Close();
#if defined(SRCFILE)
    fp.close();
#endif
    std::cout << "curl file end" << std::endl;
    return 0;
}