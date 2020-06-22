#include "PlayInternal.h"

namespace duobei {
namespace video {

bool PlayInternal::Init(std::string const& stream_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    assert(handle == nullptr);
    auto h = AVCallback::initVideoPlayer(stream_id, &player);
    if (h == nullptr) {
        return false;
    }
    handle = h;
    id = stream_id;
    return true;
}

bool PlayInternal::Destroy() {
    std::lock_guard<std::mutex> lock(mtx_);
    assert(handle);
    AVCallback::destroyVideoPlayer(handle);
    handle = nullptr;
    id.clear();
    return true;
}

void PlayInternal::Play(void* data, uint32_t size, int w, int h, uint32_t timestamp) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (handle) {
        player(handle, data, size, w, h, timestamp);
    }
}
}

namespace audio {

bool PlayInternal::Init(std::string const& stream_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    assert(handle == nullptr);
    auto h = AVCallback::initPcmPlayer(stream_id, &player);
    assert(h);
    if (h == nullptr) {
        return false;
    }
    handle = h;
    id = stream_id;
    return true;
}

bool PlayInternal::Destroy() {
    std::lock_guard<std::mutex> lock(mtx_);
    assert(handle);
    AVCallback::destroyPcmPlayer(handle);
    handle = nullptr;
    id.clear();
    return true;
}

void PlayInternal::Play(void* data, uint32_t size, uint32_t timestamp) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (handle) {
        player(handle, data, size, timestamp);
    }
}
}  // namespace audio
}  // namespace duobei
