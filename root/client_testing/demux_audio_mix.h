#pragma once

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

#include "Param.h"
#include "../src_code/utils/Time.h"

struct AuidoCache {
    struct AudioBuffer {
        uint8_t *data = nullptr;
        int size = 0;
    };
    std::mutex audioMx_;
    std::condition_variable audioCv_;
    std::list<AudioBuffer> audioBufferList;

    struct PCM {
        enum { kChunk = 640 };
        uint8_t buffer[kChunk] = {0};
        int postion = 0;
    } pcm;
    void chunkMix(uint8_t *data, int size) {
        int postion = 0;
        while (postion < size) {
            auto copy_length = std::min(pcm.kChunk - pcm.postion, size - postion);
            memcpy(pcm.buffer + pcm.postion, static_cast<uint8_t*>(data) + postion, copy_length);
            postion += copy_length;
            pcm.postion += copy_length;

            if (pcm.postion == pcm.kChunk) {
                // todo:
                AudioBuffer audio;
                audio.size = pcm.kChunk;
                audio.data = new uint8_t[audio.size];
                memcpy(audio.data, pcm.buffer, audio.size);
                audioBufferList.push_back(audio);
                pcm.postion = 0;
            }
        }
    }

    void pushCache(uint8_t *data, int size) {
        std::lock_guard<std::mutex> lckMap(audioMx_);
        chunkMix(data, size);
        audioCv_.notify_all();
    }

    int popCache(uint8_t *data) {
        if (audioBufferList.empty()) {
            std::unique_lock<std::mutex> lckMap(audioMx_);
            // todo: audioCv_.wait(lckMap, __pred)
            duobei::Time::Timestamp begin;
            begin.Start();
            audioCv_.wait(lckMap, [&]() mutable->bool {
                begin.Stop();
                return begin.Elapsed() > 1000;
            });
            if (audioBufferList.empty()) {
                std::cout << "time out" << std::endl;
                return -1;
            }
        }
        std::unique_lock<std::mutex> lckMap(audioMx_);
        assert(!audioBufferList.empty());
        auto audio = audioBufferList.front();
        memcpy(data, audio.data, audio.size);
        delete[] audio.data;
        audioBufferList.pop_front();
        return audio.size;
    }
};

static AuidoCache cache_1;
static AuidoCache cache_2;

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
        uint8_t *resampleData = new uint8_t[bufferSize];
        fillFrame(resampleData, bufferSize);
        // todo: 会崩溃
        // delete[] resampleData;
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

    bool seekTo(uint32_t time) {
        return true;
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
        assert(resample_size > 0);
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
    Decodec() = default;

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
        av_packet_free(&pkt);
    }

public:
    void start(std::string url_1, std::string url_2) {
        demux_thread_1 = std::thread(&Api::demuxer_thread, this, url_1, 1);
        demux_thread_2 = std::thread(&Api::demuxer_thread, this, url_2, 2);
    }

    void stop() {
        demux_thread_1.join();
        demux_thread_2.join();
    }

    bool thread_over() {
        return threadExit == 2;
    }
};

int demux_audio_mix(Argument &cmd);