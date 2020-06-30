#pragma once

#include "../util/Callback.h"
#include "../util/DBLog.h"

#include <mutex>

namespace duobei {
namespace video {
struct PlayInternal {
    std::mutex mtx_;
    void* handle = nullptr;
    std::string id;
    AVCallback::VideoPlayer player;
    bool Init(const std::string& stream_id);
    bool Destroy();
    void Play(void* data, uint32_t size, int w, int h, uint32_t timestamp);
};

}

namespace audio {
struct PlayInternal {
    std::mutex mtx_;
    void* handle = nullptr;
    std::string id;
    AVCallback::PcmPlayer player;
    bool Init(const std::string& stream_id);
    bool Destroy();
    void Play(void* data, uint32_t size, uint32_t timestamp);
};

}
}
