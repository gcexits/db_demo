#pragma once

#include <iostream>
#include <fstream>
#include <map>
#include <queue>
#include <set>

#include "../flv_parse/hlring/RingBuffer.h"
#include "Media.h"
#include "VideoDisplay.h"

#include <SDL2/SDL.h>

struct AudioChannel {
    std::mutex mtx_;
    std::string uid;
    RingBuffer buffer_;

    explicit AudioChannel(const std::string& uid) : uid(uid), buffer_(uid, 320 * 40) {}

    void push(void* data, uint32_t size) {
        std::lock_guard<std::mutex> lock(mtx_);
        buffer_.write(data, size);
    }
    void clear() {
        buffer_.clean();
    }
};

struct VideoChannel {
    Uint32 window_id;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_Rect rect;
    std::string uid;
    std::mutex mtx_;

    explicit VideoChannel(const std::string& uid) : uid(uid) {}

    bool ScreenSameIn(int w, int h) const {
        return std::abs(rect.w - w) < 10 && std::abs(rect.h - h) < 10;
    }

    bool ScreenVaild() const {
        return rect.w != 0 && rect.h != 0;
    }
    void Destroy() {
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyTexture(texture);
        window = nullptr;
        renderer = nullptr;
        texture = nullptr;
        rect.w = 0;
        rect.h = 0;
    }
    bool Create(int width, int height) {
        window = SDL_CreateWindow(uid.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  width, height, SDL_WINDOW_OPENGL);
        if (!window) {
            printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
            return false;
        }
        rect.w = width;
        rect.h = height;
        rect.x = 0;
        rect.y = 0;
        window_id = SDL_GetWindowID(window);
        renderer = SDL_CreateRenderer(window, -1, 0);
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, rect.w, rect.h);

        return true;
    }

    void Update(AVFrame* data) {
        SDL_UpdateYUVTexture(texture, nullptr, data->data[0], data->linesize[0],
                             data->data[1], data->linesize[1],
                             data->data[2], data->linesize[2]);
        SDL_RenderCopy(renderer, texture, nullptr, &rect);
        SDL_RenderPresent(renderer);
    }
};

struct AudioContainer {
    std::set<std::unique_ptr<AudioChannel>> channels_;
    std::mutex mtx;
    static constexpr int cache_len = 1024 * 8;
    uint8_t cache[cache_len];

    void MixAudio(Uint8* stream, int len) {
        SDL_memset(stream, 0, len);
        std::lock_guard<std::mutex> lock(mtx);

        for (auto& x : channels_) {
            std::lock_guard<std::mutex> lock(x->mtx_);
            if (x->buffer_.size() == 0) {
                continue;
            }
            len = len > x->buffer_.size() ? x->buffer_.size() : len;
            auto l = x->buffer_.read(cache, len);
            SDL_MixAudio(stream, cache, l, SDL_MIX_MAXVOLUME);
        }
    }

    void* add(const std::string& uid) {
        std::lock_guard<std::mutex> lock(mtx);
        auto a = std::make_unique<AudioChannel>(uid);
        auto r = channels_.insert(std::move(a));
        return r.second ? r.first->get() : nullptr;
    }

    void erase(void* that) {
        std::lock_guard<std::mutex> lock(mtx);
        auto iter = std::find_if(channels_.begin(), channels_.end(),
                                 [&that](const std::unique_ptr<AudioChannel>& x) -> bool {
                                     return static_cast<void*>(x.get()) == that;
                                 });
        channels_.erase(iter);
    }
};

struct VideoContainer {
    std::mutex mtx;
    std::set<VideoChannel*> sources;
    std::map<Uint32, VideoChannel*> channels_;
    MediaState* mediaState = nullptr;
    std::ofstream fp;

    VideoChannel* add(const std::string& uid) {
        auto item = new VideoChannel(uid);
        std::lock_guard<std::mutex> lock(mtx);
        sources.insert(item);
        return item;
    }
    void erase(VideoChannel* that) {
        std::lock_guard<std::mutex> lock(mtx);
        sources.erase(that);
    }

    void* remove(Uint32 window_id) {
        auto iter = channels_.find(window_id);
        void* p = nullptr;
        if (iter != channels_.end()) {
            p = iter->second;
            channels_.erase(iter);
        }
        return p;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mtx);
        for (auto& iter : sources) {
            clearBuffer(iter);
        }
        sources.clear();
        channels_.clear();
    }

    void write(AVFrame *frame) {
        int h = 1080;
        int w = 1920;

        if (!fp.is_open()) {
            fp.open("/Users/guochao/1.yuv", std::ios::binary | std::ios::out | std::ios::app);
        }
        char* buf = new char[h * w * 1.5];
        int a = 0;
        for (int i = 0; i < h; i++) {
            memcpy(buf + a, frame->data[0] + i * frame->linesize[0], w);
            a += w;
        }
        for (int i = 0; i < h / 2; i++) {
            memcpy(buf + a, frame->data[1] + i * frame->linesize[1], w / 2);
            a += w / 2;
        }
        for (int i = 0; i < h / 2; i++) {
            memcpy(buf + a, frame->data[2] + i * frame->linesize[2], w / 2);
            a += w / 2;
        }
        fp.write(buf, w * h * 1.5);
        delete[] buf;
        buf = NULL;
    }

    bool show_internal(VideoChannel* channel) {
        VideoState* video = mediaState->video;

        if (video->videoq->queue.empty())
            schedule_refresh(mediaState, 1);
        else {
            video->frameq.deQueue(&video->frame);

            // 将视频同步到音频上，计算下一帧的延迟时间
            double current_pts = *(double*)video->frame->opaque;
            double delay = current_pts - video->frame_last_pts;
            if (delay <= 0 || delay >= 1.0)
                delay = video->frame_last_delay;

            video->frame_last_delay = delay;
            video->frame_last_pts = current_pts;

            // 当前显示帧的PTS来计算显示下一帧的延迟
            double ref_clock = mediaState->audio->get_audio_clock();

            double diff = current_pts - ref_clock;  // diff < 0 => video slow,diff > 0 => video quick

            double threshold = (delay > SYNC_THRESHOLD) ? delay : SYNC_THRESHOLD;

            if (fabs(diff) < NOSYNC_THRESHOLD) {
                if (diff <= -threshold)  // 慢了，delay设为0
                    delay = 0;
                else if (diff >= threshold)  // 快了，加倍delay
                    delay *= 2;
            }
            video->frame_timer += delay;
            double actual_delay = video->frame_timer - static_cast<double>(av_gettime()) / 1000000.0;
            if (actual_delay <= 0.010)
                actual_delay = 0.010;

            std::cout << "delay time : " << static_cast<int>(actual_delay * 1000 + 0.5) << std::endl;
            schedule_refresh(mediaState, static_cast<int>(actual_delay * 1000 + 0.5));

            if (channel->ScreenVaild() && !channel->ScreenSameIn(video->frame->width, video->frame->height)) {
                channel->Destroy();
            }

            if (!channel->ScreenVaild()) {
                if (!channel->Create(video->frame->width, video->frame->height)) {
                    return false;
                }
                channels_.emplace(channel->window_id, channel);
            }

            channel->Update(video->frame);
            av_frame_unref(video->frame);
        }

        return true;
    }

    void clearBuffer(VideoChannel* channel) {
        std::unique_lock<std::mutex> lock_(channel->mtx_);
        channel->Destroy();
    }

    bool show() {
        std::lock_guard<std::mutex> lock(mtx);
        for (auto& v : sources) {
            show_internal(v);
        }
        return true;
    }

    void exit() {
        clear();
        fp.close();
    }
};

class SDLPlayer {
public:
    explicit SDLPlayer();
    static SDLPlayer* player;

    SDL_AudioSpec audioSpec;
    VideoContainer videoContainer;
    AudioContainer audioContainer;

    static void AudioCallback(void* userdata, Uint8* stream, int len) {
        auto that = (SDLPlayer*)userdata;
        that->audioContainer.MixAudio(stream, len);
    }

    bool running = true;
    static SDLPlayer* getPlayer();
    virtual ~SDLPlayer() = default;

    void EventLoop();

    void playAudio(int channels, int sample_rate, int nb_samples) {
        if (audioSpec.channels == channels && audioSpec.freq == sample_rate && audioSpec.samples == nb_samples) {
            return;
        }
        audioSpec.freq = sample_rate;
        audioSpec.format = AUDIO_S16SYS;
        audioSpec.channels = channels;
        audioSpec.silence = 0;
        audioSpec.samples = nb_samples;
        audioSpec.callback = AudioCallback;
        audioSpec.userdata = this;  //可以直接在内部传值给callback函数
        SDL_OpenAudio(&audioSpec, NULL);
        SDL_PauseAudio(0);
    }

    void do_exit() {
        videoContainer.exit();
        SDL_CloseAudio();
        SDL_Quit();
        running = false;
    }
};