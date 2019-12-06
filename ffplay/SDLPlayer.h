#pragma once

#include <iostream>
#include <map>
#include <queue>
#include <set>

#include "Optional.h"

#include "../root/src_code/hlring/RingBuffer.h"
#include "VideoDisplay.h"
#include "H264Decoder.h"

#include <SDL2/SDL.h>

struct AudioChannel {
    struct AudioState_ {
        int64_t audio_buff_consume;
        int64_t decode_size;
        int channels;
        double audio_clock; // audio clock
        AVStream *stream; // audio stream
        AudioState_() {
            audio_buff_consume = 0;
            decode_size = 0;
            channels = 2;
            audio_clock = 0;
            stream = nullptr;
        }
        ~AudioState_() {
            if (stream) {
                stream = nullptr;
            }
        }

    };

    AudioState_ audioState;
    std::mutex mtx_;
    std::string uid;
    RingBuffer buffer_;

    explicit AudioChannel(const std::string& uid) : uid(uid), buffer_(uid, 0) {}

    void push(void* data, uint32_t size, int64_t pts) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (buffer_.size() > 1024 * 1024 * 1024) {
            std::cout << "date send too quick" << std::endl;
            abort();
        }
        buffer_.write(data, size);
        audioState.audio_clock = av_q2d(audioState.stream->time_base) * pts;
        audioState.audio_clock += static_cast<double>(size) / (2 * audioState.channels * audioState.stream->codecpar->sample_rate);
        audioState.decode_size += size;
    }
    void clear() {
        std::lock_guard<std::mutex> lock(mtx_);
        buffer_.clean();
    }

    // get audio clock
    double get_audio_clock() {
        std::lock_guard<std::mutex> lock(mtx_);
        // todo: 剩余的数据块
        int hw_buf_size = audioState.decode_size - audioState.audio_buff_consume;
        int bytes_per_sec = audioState.stream->codecpar->sample_rate * audioState.channels * 2;
        return audioState.audio_clock - static_cast<double>(hw_buf_size) / bytes_per_sec;
    }
};

struct VideoChannel {
    struct PixelBuffer {
        void* data = nullptr;
        int pitch = 0;  // The number of bytes in a row of pixel data, including padding between lines.
        int capacity = 0;
        int w = 0;
        int h = 0;
        double pts = 0;
        PixelBuffer(void* _data, int _pitch, int _w, int _h, double _p) {
            capacity = _w * _h * 3 / 2 + 1;
            data = new uint8_t[capacity];
            update(_data, _pitch, _w, _h, _p);
        }

        ~PixelBuffer() {
            if (data) {
                delete[] static_cast<uint8_t*>(data);
            }
        }

        bool update(void* _data, int _pitch, int _w, int _h, double _p) {
            int size = _w * _h * 3 / 2;
            if (size < capacity) {
                memcpy(data, _data, size);
                pitch = _w;
                w = _w;
                h = _h;
                pts = _p;
                return true;
            }
            return false;
        }

        using Ptr = std::unique_ptr<PixelBuffer>;
    };

    double frame_timer = static_cast<double>(av_gettime()) / 1000000.0;         // Sync fields
    double frame_last_pts = 0;
    double frame_last_delay = 40e-3;

    Uint32 window_id;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_Rect rect;
    std::string uid;
    std::queue<PixelBuffer::Ptr> work_queue_;
    std::queue<PixelBuffer::Ptr> ready_queue_;
    std::mutex mtx_;

    explicit VideoChannel(const std::string& uid) : uid(uid) {}

    bool ScreenSameIn(int w, int h) const {
        return std::abs(rect.w - w) < 10 && std::abs(rect.h - h) < 10;
    }

    bool ScreenSameIn(const PixelBuffer::Ptr& pixel_buffer) const {
        return std::abs(rect.w - pixel_buffer->w) < 10 && std::abs(rect.h - pixel_buffer->h) < 10;
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

    bool Create(const PixelBuffer::Ptr& pixel_buffer) {
        window = SDL_CreateWindow(uid.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  pixel_buffer->w, pixel_buffer->h, SDL_WINDOW_OPENGL);
        if (!window) {
            printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
            return false;
        }
        rect.w = pixel_buffer->w;
        rect.h = pixel_buffer->h;
        rect.x = 0;
        rect.y = 0;
        window_id = SDL_GetWindowID(window);
        renderer = SDL_CreateRenderer(window, -1, 0);
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, rect.w, rect.h);

        return true;
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

    void Update(const PixelBuffer::Ptr& pixel_buffer) {
        assert(ScreenSameIn(pixel_buffer));
        SDL_UpdateTexture(texture, nullptr, pixel_buffer->data, pixel_buffer->pitch);
        SDL_RenderCopy(renderer, texture, nullptr, &rect);
        SDL_RenderPresent(renderer);
    }

    void push(void* data, uint32_t size, int w, int h, double pts) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (ready_queue_.empty()) {
            work_queue_.emplace(new PixelBuffer(data, size, w, h, pts));
        } else {
            auto& e = ready_queue_.front();
            if (e->update(data, size, w, h, pts)) {
                work_queue_.push(std::move(e));
            } else {
                work_queue_.emplace(new PixelBuffer(data, size, w, h, pts));
            }
            ready_queue_.pop();
        }
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
            while (len > 0) {
                if (x->buffer_.size() == 0) {
                    continue;
                }
                auto len1 = x->buffer_.size() > len ? len : x->buffer_.size();

                std::lock_guard<std::mutex> lock(x->mtx_);
                x->buffer_.read(cache, len1);

                SDL_MixAudio(stream, cache, len1, SDL_MIX_MAXVOLUME);

                len -= len1;
                stream += len1;

                // todo: 消耗的数据块
                x->audioState.audio_buff_consume += len1;
            }
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

    void clearBuffer(VideoChannel* channel) {
        std::unique_lock<std::mutex> lock_(channel->mtx_);
        channel->Destroy();
    }

    void exit() {
        clear();
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
        audioSpec.userdata = this;
        SDL_OpenAudio(&audioSpec, NULL);
        SDL_PauseAudio(0);
    }

    void* openVideo(const std::string& uid, AVRegister::VideoPlayer* f) {
        using namespace std::placeholders;
        *f = std::bind(&SDLPlayer::pushVideoData, this, _1, _2, _3, _4, _5, _6);
        return videoContainer.add(uid);
    }

    void pushVideoData(void* handle, void* data, uint32_t size, int w, int h, double pts) {
        assert(handle);
        auto that = static_cast<VideoChannel*>(handle);
        that->push(data, size, w, h, pts);
    }

    void closeVideo(void* handle) {
    }

    // initPcmPlayer
    void* openAudio(const std::string& uid, AVRegister::PcmPlayer* f) {
        using namespace std::placeholders;
        *f = std::bind(&SDLPlayer::pushAudioData, this, _1, _2, _3, _4);
        return audioContainer.add(uid);
    }

    void pushAudioData(void* handle, void* data, uint32_t size, int64_t pts) {
        assert(handle);
        auto that = static_cast<AudioChannel*>(handle);
        that->push(data, size, pts);
    }

    // destroyPcmPlayer
    void closeAudio(void* handle) {
    }

    void do_exit() {
        videoContainer.exit();
        SDL_CloseAudio();
        SDL_Quit();
        running = false;
    }

    bool show_internal(VideoChannel* videoChannel, AudioChannel *audioChannel) {
        std::unique_lock<std::mutex> lock_(videoChannel->mtx_);
        if (videoChannel->work_queue_.empty()) {
            schedule_refresh(1);
            return false;
        }
        auto pixel_buffer = std::move(videoChannel->work_queue_.front());
        videoChannel->work_queue_.pop();
        lock_.unlock();

        double current_pts = pixel_buffer->pts;
        double delay = current_pts - videoChannel->frame_last_pts;
        if (delay <= 0 || delay >= 1.0)
            delay = videoChannel->frame_last_delay;

        videoChannel->frame_last_delay = delay;
        videoChannel->frame_last_pts = current_pts;

        // 当前显示帧的PTS来计算显示下一帧的延迟
        double ref_clock = audioChannel->get_audio_clock();

        double diff = current_pts - ref_clock;  // diff < 0 => video slow,diff > 0 => video quick

        double threshold = (delay > SYNC_THRESHOLD) ? delay : SYNC_THRESHOLD;

        if (fabs(diff) < NOSYNC_THRESHOLD) {
            if (diff <= -threshold) {
                // 慢了，delay设为0
                delay = 0;
            } else if (diff >= threshold) {
                // 快了，加倍delay
                delay *= 2;
            }
        }
        videoChannel->frame_timer += delay;
        double actual_delay = videoChannel->frame_timer - static_cast<double>(av_gettime()) / 1000000.0;
        if (actual_delay <= 0.010)
            actual_delay = 0.010;

//        std::cout << "delay time : " << static_cast<int>(actual_delay * 1000 + 0.5) << std::endl;
        schedule_refresh(static_cast<int>(actual_delay * 1000 + 0.5));
        if (videoChannel->ScreenVaild() && !videoChannel->ScreenSameIn(pixel_buffer)) {
            videoChannel->Destroy();
        }
        if (!videoChannel->ScreenVaild()) {
            if (!videoChannel->Create(pixel_buffer)) {
                return false;
            }
//            channels_.emplace(channel->window_id, channel);
        }
        videoChannel->Update(pixel_buffer);
        videoChannel->ready_queue_.push(std::move(pixel_buffer));
        return true;
    }

    bool show() {
        show_internal(*videoContainer.sources.begin(), (*audioContainer.channels_.begin()).get());
//        for (auto& v : videoContainer.sources) {
//            for (auto& a : audioContainer.channels_) {
//                if (a->uid == v->uid) {
//                    show_internal(v, a.get());
////                    videoContainer.channels_.emplace(v->window_id, v);
//                }
//            }
//        }
        return true;
    }
};