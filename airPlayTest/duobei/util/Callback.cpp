#include "Callback.h"

namespace duobei {

namespace internal {

namespace detail {
static std::function<void(int level, const char* timestamp, const char* fn, int line, const char* msg)> prettyLog = nullptr;
}

namespace AVCallbackImpl {

using Handle = AVCallback::Handle;
using Destroyer = AVCallback::Destroyer;

using VideoPlayer = AVCallback::VideoPlayer;
static std::function<Handle(const std::string&, VideoPlayer*)> initVideoPlayer = nullptr;
static Destroyer destroyVideoPlayer = nullptr;

using PcmPlayer = AVCallback::PcmPlayer;
static std::function<Handle(const std::string&, PcmPlayer*)> initPcmPlayer = nullptr;
static Destroyer destroyPcmPlayer = nullptr;
}

}


namespace Callback {
void prettyLog(int level, const char* timestamp, const char* fn, int line, const char* msg) {
    if (!internal::detail::prettyLog) {
        return;
    }
    internal::detail::prettyLog(level, timestamp, fn, line, msg);
}
}

namespace RegisterCallback {
void setprettyLogCallback(std::function<void(int level, const char* timestamp, const char* fn, int line, const char* msg)> f) {
    internal::detail::prettyLog = std::move(f);
}

}

namespace AVCallback {

Handle initVideoPlayer(const std::string& stream_id, VideoPlayer* f) {
    if (!internal::AVCallbackImpl::initVideoPlayer) {
        return nullptr;
    }
    return internal::AVCallbackImpl::initVideoPlayer(stream_id, f);
}

Handle initPcmPlayer(const std::string& stream_id, PcmPlayer* f) {
    if (!internal::AVCallbackImpl::initPcmPlayer) {
        return nullptr;
    }
    return internal::AVCallbackImpl::initPcmPlayer(stream_id, f);
}

void destroyPcmPlayer(Handle handle) {
    if (!internal::AVCallbackImpl::destroyPcmPlayer) {
        return;
    }
    internal::AVCallbackImpl::destroyPcmPlayer(handle);
    return;
}

void destroyVideoPlayer(Handle handle) {
    if (!internal::AVCallbackImpl::destroyVideoPlayer) {
        return;
    }
    internal::AVCallbackImpl::destroyVideoPlayer(handle);
    return;
}

}

namespace AVRegister {

void setdestroyPcmPlayer(Destroyer f) {
    internal::AVCallbackImpl::destroyPcmPlayer = std::move(f);
}

void setdestroyVideoPlayer(Destroyer f) {
    internal::AVCallbackImpl::destroyVideoPlayer = std::move(f);
}

void setinitPcmPlayer(std::function<Handle(const std::string& id, PcmPlayer*)> f) {
    internal::AVCallbackImpl::initPcmPlayer = std::move(f);
}

void setinitVideoPlayer(std::function<Handle(const std::string& id, VideoPlayer*)> f) {
    internal::AVCallbackImpl::initVideoPlayer = std::move(f);
}

}
}
