extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <thread>

struct AuidoCache {
public:
    struct AudioBuffer {
        uint8_t *data = nullptr;
        int size = 0;
    };
    std::mutex audioMx_;
    std::list<AudioBuffer> audioBufferList;

    void pushCache(uint8_t *data, int size) {
        AudioBuffer audio;
        audio.size = size;
        audio.data = new uint8_t[size];
        memcpy(audio.data, data, size);
        std::lock_guard<std::mutex> lckMap(audioMx_);
        audioBufferList.push_back(audio);
    }
};

class Demux {
    AVFormatContext *ifmt = nullptr;
    std::string url;
    int audio_index = -1;
    int video_index = -1;
    AVPacket *pkt;

public:
    Demux(std::string inUrl) : url(inUrl) {
        av_register_all();
        pkt = av_packet_alloc();
    }
    ~Demux() {
        if (ifmt) {
            avformat_close_input(&ifmt);
        }
        if (pkt) {
            av_packet_free(&pkt);
        }
    }

    bool demuxFormat() {
        if (avformat_open_input(&ifmt, url.c_str(), nullptr, nullptr) != 0) {
            return false;
        }
        avformat_find_stream_info(ifmt, nullptr);
        audio_index = av_find_best_stream(ifmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        video_index = av_find_best_stream(ifmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (audio_index == -1 && video_index == -1) {
            return false;
        }
        return true;
    }

    int64_t getDuration() {
        return ifmt->duration / AV_TIME_BASE;
    }

    int getAudioSize() {
        if (audio_index == -1) {
            return -1;
        }
        return ifmt->streams[audio_index]->codecpar->frame_size;
    }

    AVCodecParameters *getAudioParameters() {
        if (audio_index == -1) {
            return nullptr;
        }
        return ifmt->streams[audio_index]->codecpar;
    }

    bool readFrame() {
        return av_read_frame(ifmt, pkt) == 0 ? true : false;
    }

    int streamType() {
        return pkt->stream_index == video_index ? 1 : 0;
    }

    void getPkt(AVPacket *dst) {
        av_packet_ref(dst, pkt);
        av_packet_unref(pkt);
    }
};

class Decodec {
    AVCodecContext *ctx = nullptr;
    AVFrame *frame = nullptr;
    SwrContext *swr = nullptr;

public:
    AuidoCache cache;
    Decodec() {
        avcodec_register_all();
        frame = av_frame_alloc();
    }

    ~Decodec() {
        if (ctx) {
            avcodec_free_context(&ctx);
        }
        if (frame) {
            av_frame_free(&frame);
        }
    }

    bool setCodec(AVCodecParameters *par) {
        AVCodec *codec = avcodec_find_decoder(par->codec_id);
        if (!codec) {
            std::cout << __LINE__ << std::endl;
            return false;
        }
        ctx = avcodec_alloc_context3(codec);
        if (!ctx) {
            std::cout << __LINE__ << std::endl;
            return false;
        }
        avcodec_parameters_to_context(ctx, par);
        return true;
    }

    bool openCodec() {
        return avcodec_open2(ctx, nullptr, nullptr) == 0 ? true : false;
    }

    bool streamDecode(AVPacket *pkt) {
        if (avcodec_send_packet(ctx, pkt) != 0) {
            return false;
        }
        while (true) {
            if (avcodec_receive_frame(ctx, frame) != 0) {
                break;
            }
            initSwr(frame->channels, frame->sample_rate, (AVSampleFormat)frame->format);
            int size = frame->nb_samples * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * 2;
            uint8_t *pcm = new uint8_t[size];
            swr_convert(swr, &pcm, frame->nb_samples, (const uint8_t **)frame->data, frame->nb_samples);
            cache.pushCache(pcm, size);
            delete[] pcm;
        }
        return true;
    }

    void initSwr(int in_channels, int in_sample_rate, AVSampleFormat in_sample_format) {
        swr = swr_alloc();
        swr = swr_alloc_set_opts(swr, av_get_default_channel_layout(2), AV_SAMPLE_FMT_S16, in_sample_rate, av_get_default_channel_layout(in_channels), in_sample_format, in_sample_rate, 0, nullptr);
        swr_init(swr);
    }
};

// todo : audio解封装，解码
int main(int argc, char *argv[]) {
    std::string outUrl = "./decode.pcm";
    std::ofstream fp_pcm;
    fp_pcm.open(outUrl, std::ios::out | std::ios::app);
    std::string url = "../audio_video/盗将行.mp3";

    // av_log_set_level(AV_LOG_QUIET);

    Demux demuxer(url);
    demuxer.demuxFormat();

    std::cout << "audio_video duration(second) : " << demuxer.getDuration() << std::endl;

    // av_dump_format(ifmt, -1, url.c_str(), 0);

    std::cout << "audio frame_size(nb_samples 1 channel) : " << demuxer.getAudioSize() << std::endl;

    Decodec audioCtx;
    if (!audioCtx.setCodec(demuxer.getAudioParameters())) {
        std::cout << __LINE__ << std::endl;
        return -1;
    }
    if (!audioCtx.openCodec()) {
        std::cout << __LINE__ << std::endl;
        return -1;
    }

    AVPacket *pkt = av_packet_alloc();
    while (1) {
        if (!demuxer.readFrame()) {
            std::cout << __LINE__ << std::endl;
            break;
        }

        if (demuxer.streamType() == 0) {
            demuxer.getPkt(pkt);
            audioCtx.streamDecode(pkt);
        }

        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    while (!audioCtx.cache.audioBufferList.empty()) {
        auto lv = audioCtx.cache.audioBufferList.front();
        fp_pcm.write((char *)lv.data, lv.size);
        delete[] lv.data;
        audioCtx.cache.audioBufferList.pop_front();
    }

    fp_pcm.close();

    return 0;
}