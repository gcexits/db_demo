#pragma once

#include <map>
#include <queue>
#include <set>

#include "../utils/Optional.h"
#include "../hlring/RingBuffer.h"

#include <SDL2/SDL.h>

struct AudioChannel {
    std::mutex mtx_;
    std::string uid;
    RingBuffer buffer_;

    explicit AudioChannel(const std::string& uid) : uid(uid), buffer_(uid, 0) {}

    void push(void* data, uint32_t size, double pts) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (buffer_.size() > 1024 * 1024 * 1024) {
            std::cout << "date send too quick" << std::endl;
            abort();
        }
        buffer_.write(data, size);
    }
    void clear() {
        std::lock_guard<std::mutex> lock(mtx_);
        buffer_.clean();
    }
};

struct VideoChannel {
    struct PixelBuffer {
        void* data = nullptr;
        int pitch = 0;  // The number of bytes in a row of pixel data, including padding between lines.
        int capacity = 0;
        int w = 0;
        int h = 0;
        double pts = 0.0;
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

    void* find(Uint32 window_id) {
        auto iter = channels_.find(window_id);
        if (iter == channels_.end()) {
            return nullptr;
        }
        SDL_GetWindowSize(iter->second->window, &iter->second->rect.w, &iter->second->rect.h);
        return iter->second;
    }

    bool show_internal(VideoChannel* channel) {
        while (true) {
            std::unique_lock<std::mutex> lock_(channel->mtx_);
            if (channel->work_queue_.empty()) {
                break;
            }
            auto pixel_buffer = std::move(channel->work_queue_.front());
            channel->work_queue_.pop();
            lock_.unlock();

            if (channel->ScreenVaild() && !channel->ScreenSameIn(pixel_buffer)) {
                channel->Destroy();
            }
            if (!channel->ScreenVaild()) {
                if (!channel->Create(pixel_buffer)) {
                    break;
                }
                channels_.emplace(channel->window_id, channel);
            }

            channel->Update(pixel_buffer);
            channel->ready_queue_.push(std::move(pixel_buffer));
        }
        return true;
    }

    void clearBuffer(VideoChannel* channel) {
        std::unique_lock<std::mutex> lock_(channel->mtx_);
        while (channel->work_queue_.empty()) {
            channel->work_queue_.pop();
        }
        while (channel->ready_queue_.empty()) {
            channel->ready_queue_.pop();
        }
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
    }
};

class SDLPlayer {
public:
    explicit SDLPlayer();
    static SDLPlayer* player;

    SDL_AudioSpec audioSpec;
    AudioContainer audioContainer;
    VideoContainer videoContainer;

    static int RefreshLoop(void* arg);
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

    void pushAudioData(void* handle, void* data, uint32_t size, double pts) {
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
};
