#pragma once

#include <iostream>

#define USING_SDL

namespace AVCallback {
using VideoPlayer = std::function<void(void*, void*, uint32_t, int, int, double pts)>;
void* initVideoPlayer(const std::string& uid, VideoPlayer* f);

using PcmPlayer = std::function<void(void*, void*, uint32_t, int64_t pts)>;
void* initPcmPlayer(const std::string& uid, PcmPlayer* f);

void destroyPcmPlayer(void* handle);
void destroyVideoPlayer(void* handle);
}  // namespace AVCallback

namespace AVRegister {
using VideoPlayer = AVCallback::VideoPlayer;
void setinitVideoPlayer(std::function<void*(const std::string& uid, VideoPlayer*)> f);

using PcmPlayer = AVCallback::PcmPlayer;
void setinitPcmPlayer(std::function<void*(const std::string& uid, PcmPlayer*)> f);

void setdestroyPcmPlayer(std::function<void(void*)> f);
void setdestroyVideoPlayer(std::function<void(void*)> f);
}  // namespace AVRegister

namespace audio {
struct PlayInternal {
    std::mutex mtx_;
    void* handle = nullptr;
    AVCallback::PcmPlayer player;

    bool Init(const std::string& uid) {
        std::lock_guard<std::mutex> lock(mtx_);
        assert(handle == nullptr);
        auto h = AVCallback::initPcmPlayer(uid, &player);
        if (h == nullptr) {
            return false;
        }
        handle = h;
        return true;
    }

    void Destroy() {
        std::lock_guard<std::mutex> lock(mtx_);
        assert(handle);
        AVCallback::destroyPcmPlayer(handle);
        handle = nullptr;
    }

    void Play(void* data, uint32_t size, int64_t pts) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (handle) {
            player(handle, data, size, pts);
        }
    }
};
}  // namespace audio

namespace video {
struct PlayInternal {
    std::mutex mtx_;
    void* handle = nullptr;
    AVCallback::VideoPlayer player;
    bool Init(const std::string& uid) {
        std::lock_guard<std::mutex> lock(mtx_);
        assert(handle == nullptr);
        auto h = AVCallback::initVideoPlayer(uid, &player);
        if (h == nullptr) {
            return false;
        }
        handle = h;
        return true;
    }
    void Destroy() {
        std::lock_guard<std::mutex> lock(mtx_);
        assert(handle);
        AVCallback::destroyVideoPlayer(handle);
        handle = nullptr;
    }

    void Play(void* data, uint32_t size, int w, int h, double pts) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (handle) {
            player(handle, data, size, w, h, pts);
        }
    }
};
}  // namespace video