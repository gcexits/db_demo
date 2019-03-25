extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

#include <cassert>
#include <fstream>
#include <iostream>
#include <list>
#include <mutex>
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

    int popCache(uint8_t *data) {
        std::lock_guard<std::mutex> lckMap(audioMx_);
        if (audioBufferList.empty()) {
            return 0;
        }
        auto audio = audioBufferList.front();
        memcpy(data, audio.data, audio.size);
        delete[] audio.data;
        audioBufferList.pop_front();
        return audio.size;
    }
};

AuidoCache cache_1;
AuidoCache cache_2;

class AudioData {
    void allocFrame() {
        if (!frame) {
            frame = av_frame_alloc();
        }
    }

    void resertFrame() {
        if (frame) {
            av_frame_free(&frame);
        }
    }

public:
    AVFrame *frame = nullptr;
    uint8_t *resampleData = nullptr;
    int bufferSize = 0;
    AudioData() {
        allocFrame();
    }

    ~AudioData() {
        resertFrame();
    }

    void setOpt(int nb_samples, int sample_rate, int channels, int format) {
        frame->nb_samples = nb_samples;
        frame->sample_rate = sample_rate;
        frame->channels = channels;
        frame->format = format;
    }

    void fillFrame(uint8_t *audioBuffer, int bufferSize) {
        avcodec_fill_audio_frame(frame, frame->channels, (AVSampleFormat)frame->format, audioBuffer, bufferSize, 0);
    }

    void fillFrame() {
        bufferSize = av_samples_get_buffer_size(nullptr, frame->channels, frame->nb_samples, (AVSampleFormat)frame->format, 0);
        resampleData = new uint8_t[bufferSize];
        fillFrame(resampleData, bufferSize);
        delete[] resampleData;
    }

    bool isChange(int nb_samples, int sample_rate, int channels, int format) {
        return nb_samples != frame->nb_samples || sample_rate != frame->sample_rate || channels != frame->channels || format != frame->format;
    }
};

class Demux {
    AVFormatContext *ifmt = nullptr;
    std::string url;
    int audio_index = -1;
    int video_index = -1;
    AVPacket *pkt = nullptr;

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

class ReSample {
    SwrContext *swr = nullptr;

public:
    AudioData src;
    AudioData dst;
    ReSample() = default;

    void setSwrOpt() {
        if (src.isChange(src.frame->nb_samples, src.frame->sample_rate, src.frame->channels, src.frame->format)) {
            if (swr) {
                swr_free(&swr);
            }
        }

        if (!swr) {
            swr = swr_alloc();
            swr = swr_alloc_set_opts(swr, av_get_default_channel_layout(dst.frame->channels), (AVSampleFormat)dst.frame->format, dst.frame->sample_rate, av_get_default_channel_layout(src.frame->channels), (AVSampleFormat)src.frame->format, src.frame->sample_rate, 0, nullptr);
            swr_init(swr);
        }
    }

    void convert(int streamId) {
        int resample_size = swr_convert(swr, dst.frame->data, dst.frame->nb_samples, (const uint8_t **)src.frame->data, src.frame->nb_samples);
        if (streamId == 1) {
            cache_1.pushCache(dst.frame->data[0], dst.bufferSize);
        } else {
            cache_2.pushCache(dst.frame->data[0], dst.bufferSize);
        }
    }
};

class Decodec {
    AVCodecContext *ctx = nullptr;
    bool isFirst = true;
    ReSample resample;

public:
    Decodec() {
        avcodec_register_all();
    }

    ~Decodec() {
        if (ctx) {
            avcodec_free_context(&ctx);
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

    bool streamDecode(AVPacket *pkt, int streamId) {
        if (avcodec_send_packet(ctx, pkt) != 0) {
            return false;
        }

        while (true) {
            if (avcodec_receive_frame(ctx, resample.src.frame) != 0) {
                break;
            }
            resample.src.setOpt(resample.src.frame->nb_samples, resample.src.frame->sample_rate, resample.src.frame->channels, resample.src.frame->format);
            resample.dst.setOpt(resample.src.frame->nb_samples, resample.src.frame->sample_rate, resample.src.frame->channels, 1);
            resample.dst.fillFrame();
            resample.setSwrOpt();
            if (isFirst) {
                std::cout << "src.frame->sample_rate : " << resample.src.frame->sample_rate << std::endl;
                std::cout << "src.frame->nb_samples : " << resample.src.frame->nb_samples << std::endl;
                isFirst = false;
            }
            resample.convert(streamId);
        }
        return true;
    }
};

class Api {
    std::thread demux_thread_1;
    std::thread demux_thread_2;
    int threadExit = 0;

    void demuxer_thread(std::string url, int streamId) {
        Demux demuxer(url);
        Decodec audioCtx;
        demuxer.demuxFormat();

        if (!audioCtx.setCodec(demuxer.getAudioParameters())) {
            std::cout << "err line : " << __LINE__ << std::endl;
            exit(1);
        }
        if (!audioCtx.openCodec()) {
            std::cout << "err line : " << __LINE__ << std::endl;
            exit(1);
        }

        AVPacket *pkt = av_packet_alloc();
        while (1) {
            if (!demuxer.readFrame()) {
                std::cout << "err line : " << __LINE__ << std::endl;
                break;
            }

            if (demuxer.streamType() == 0) {
                demuxer.getPkt(pkt);
                audioCtx.streamDecode(pkt, streamId);
            }

            // std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        threadExit++;
    }

public:
    void start(std::string url_1, std::string url_2) {
        demux_thread_1 = std::thread(&Api::demuxer_thread, this, url_1, 1);
        // demux_thread_2 = std::thread(&Api::demuxer_thread, this, url_2, 2);
    }

    void stop() {
        assert(demux_thread_1.joinable());
        // assert(demux_thread_2.joinable());
        demux_thread_1.join();
        // demux_thread_2.join();
    }

    bool threadOver() {
        return threadExit == 1;
    }
};

int main(int argc, char *argv[]) {
    std::string outUrl = "./44100_2_s16le.pcm";
    std::ofstream fp_pcm;
    fp_pcm.open(outUrl, std::ios::out | std::ios::app);
    std::string url_1 = "../audio_video/1_意外.mp3";
    std::string url_2 = "../audio_video/2_盗将行.mp3";

    Api api;
    api.start(url_1, url_2);

    uint8_t *buf_mix_1 = new uint8_t[1024 * 1024];
    uint8_t *buf_mix_2 = new uint8_t[1024 * 1024];
    uint8_t *buf_out = new uint8_t[1024 * 1024];

    while (1) {
        int buf_len_1 = cache_1.popCache(buf_mix_1);
        if (buf_len_1 == 0) {
            memset(buf_mix_1, 0, 1024 * 1024);
        }
        int buf_len_2 = cache_2.popCache(buf_mix_2);
        if (buf_len_2 == 0) {
            memset(buf_mix_2, 0, 1024 * 1024);
        }
        if (buf_len_1 == 0 && buf_len_2 == 0 && api.threadOver()) {
            break;
        }
        // fp_pcm.write((char *)buf_mix_2, buf_len_2);
        fp_pcm.write((char *)buf_mix_1, buf_len_1);
    }

    api.stop();

    delete[] buf_mix_1;
    delete[] buf_mix_2;
    delete[] buf_out;

    // av_log_set_level(AV_LOG_QUIET);

    // std::cout << "audio_video duration(second) : " << demuxer.getDuration() << std::endl;

    // std::cout << "audio frame_size(nb_samples 1 channel) : " << demuxer.getAudioSize() << std::endl;

    fp_pcm.close();

    return 0;
}