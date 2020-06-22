#pragma once

#include <functional>
#include <string>

namespace duobei {

namespace Callback {
void prettyLog(int level, const char* timestamp, const char* fn, int line, const char* msg);
}

namespace RegisterCallback {
void setprettyLogCallback(std::function<void(int, const char* timestamp, const char* fn, int line, const char* msg)> f);
}

namespace AVCallback {
using Handle = void*;
using Destroyer = std::function<void(Handle handle)>;

using VideoPlayer = std::function<void(Handle handle, void* data, uint32_t size, int w, int h, uint32_t ts)>;
Handle initVideoPlayer(const std::string& stream_id, VideoPlayer* f);
void destroyVideoPlayer(Handle handle);

using PcmPlayer = std::function<void(Handle handle, void* data, uint32_t size, uint32_t ts)>;
Handle initPcmPlayer(const std::string& stream_id, PcmPlayer* f);
void destroyPcmPlayer(Handle handle);

}

namespace AVRegister {
using Destroyer = AVCallback::Destroyer;
using Handle = AVCallback::Handle;

using VideoPlayer = AVCallback::VideoPlayer;
void setinitVideoPlayer(std::function<Handle(const std::string&, VideoPlayer*)> f);

using PcmPlayer = AVCallback::PcmPlayer;
void setinitPcmPlayer(std::function<Handle(const std::string&, PcmPlayer*)> f);

void setdestroyPcmPlayer(Destroyer f);
void setdestroyVideoPlayer(Destroyer f);

}
}
