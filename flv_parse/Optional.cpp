#include <iostream>

#include "Optional.h"

namespace AVCallbackImpl {
    using VideoPlayer = AVCallback::VideoPlayer;
    static std::function<void*(const std::string&, VideoPlayer*)> initVideoPlayer = nullptr;
    static std::function<void(void*)> destroyVideoPlayer = nullptr;

    using PcmPlayer = AVCallback::PcmPlayer;
    static std::function<void*(PcmPlayer*)> initPcmPlayer = nullptr;
    static std::function<void(void*)> destroyPcmPlayer = nullptr;
}

namespace AVCallback {
    void* initVideoPlayer(const std::string& uid, VideoPlayer* f) {
        if (!AVCallbackImpl::initVideoPlayer) {
            return nullptr;
        }
        return AVCallbackImpl::initVideoPlayer(uid, f);
    }

    void* initPcmPlayer(PcmPlayer* f) {
        if (!AVCallbackImpl::initPcmPlayer) {
            return nullptr;
        }
        return AVCallbackImpl::initPcmPlayer(f);
    }

    void destroyPcmPlayer(void* handle) {
        if (!AVCallbackImpl::destroyPcmPlayer) {
            return;
        }
        return AVCallbackImpl::destroyPcmPlayer(handle);
    }

    void destroyVideoPlayer(void* handle) {
        if (!AVCallbackImpl::destroyVideoPlayer) {
            return;
        }
        return AVCallbackImpl::destroyVideoPlayer(handle);
    }
}

namespace AVRegister {
    void setinitPcmPlayer(std::function<void*(PcmPlayer*)> f) {
        AVCallbackImpl::initPcmPlayer = std::move(f);
    }

    void setinitVideoPlayer(std::function<void*(const std::string& uid, VideoPlayer*)> f) {
        AVCallbackImpl::initVideoPlayer = std::move(f);
    }

    void setdestroyPcmPlayer(std::function<void(void*)> f) {
        AVCallbackImpl::destroyPcmPlayer = std::move(f);
    }

    void setdestroyVideoPlayer(std::function<void(void*)> f) {
        AVCallbackImpl::destroyVideoPlayer = std::move(f);
    }
}