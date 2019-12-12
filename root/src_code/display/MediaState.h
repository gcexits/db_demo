#pragma once

#include <queue>
#include <mutex>
#include <thread>

extern "C" {
#include <libavformat/avformat.h>
}

#include "../codec/VideoBuffer.h"
#include "../codec/AudioDecoder.h"
#include "../codec/H264Decoder.h"

struct PacketData {
    std::queue<AVPacket *> pq;
    std::mutex mx_;
    std::condition_variable cv_;

    void packet_queue_put(AVPacket *packet) {
        std::unique_lock<std::mutex> lck(mx_);
        AVPacket *pkt = av_packet_alloc();
        assert(av_packet_ref(pkt, packet) == 0);
        pq.push(pkt);
        cv_.notify_all();
    }

    void packet_queue_get(AVPacket *packet) {
        while (true) {
            std::unique_lock<std::mutex> lck(mx_);
            if (pq.empty()) {
                cv_.wait(lck);
            } else {
                AVPacket *pkt = pq.front();
                assert(av_packet_ref(packet, pkt) == 0);
                av_packet_unref(pkt);
                pq.pop();
                break;
            }
        }
    }
};

struct AudioState_1 {
    PacketData packetData;
    AudioDecode audioDecode;
    int64_t audio_buff_consume;
    int64_t decode_size;
    int channels;
    int sample_rate;
    double audio_clock;
    AudioState_1() {
        channels = 2;
        sample_rate = 48000;
        audio_buff_consume = 0;
        decode_size = 0;
        audio_clock = 0.0;
        decode_running = true;
    }

    ~AudioState_1() {
        exit();
    };

    void exit() {
        if (decode.joinable()) {
            decode_running = false;
            packetData.cv_.notify_all();
            decode.join();
        }
    }

    double get_audio_clock() {
        // todo: 剩余的数据块
        int hw_buf_size = decode_size - audio_buff_consume;
        int bytes_per_sec = sample_rate * channels * 2;
        return audio_clock - static_cast<double>(hw_buf_size) / bytes_per_sec;
    }

    void audio_thread() {
        while(decode_running) {
            AVPacket pkt;
            packetData.packet_queue_get(&pkt);
            audioDecode.Decode(&pkt, this);
        }
    }

    bool decode_running = false;
    std::thread decode;
};

struct VideoState_1 {
    PacketData packetData;
    H264Decode videoDecode;
    AVStream *stream;

    double video_clock;

    double synchronize(AVFrame *srcFrame, double pts) {
        double frame_delay;

        if (pts != 0)
            video_clock = pts;  // Get pts,then set video clock to it
        else
            pts = video_clock;  // Don't get pts,set it to video clock

        frame_delay = av_q2d(stream->time_base);
        frame_delay += srcFrame->repeat_pict * (frame_delay * 0.5);

        video_clock += frame_delay;

        return pts;
    }

    VideoState_1() {
        video_clock = 0.0;
        stream = nullptr;
        decode_running = true;
    }

    ~VideoState_1() {
        exit();
    };

    void exit() {
        if (decode.joinable()) {
            decode_running = false;
            packetData.cv_.notify_all();
            decode.join();
        }
    }

    void video_thread() {
        while(decode_running) {
            AVPacket pkt;
            packetData.packet_queue_get(&pkt);
            videoDecode.Decode(&pkt, this);
        }
    }

    bool decode_running = false;
    std::thread decode;
};

struct MediaState {
    AudioState_1 audioState;
    VideoState_1 videoState;

    void exit() {
        audioState.exit();
        videoState.exit();
        audioState.audioDecode.playInternal.Destroy();
        videoState.videoDecode.playInternal.Destroy();
    }
};