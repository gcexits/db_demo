#pragma once

#include <map>
#include <queue>
#include <set>

#include "../codec/H264Decoder.h"
#include "../display/MediaState.h"
#include "../hlring/RingBuffer.h"
#include "../utils/Optional.h"

#include <SDL2/SDL.h>

static const double SYNC_THRESHOLD = 0.01;
static const double NOSYNC_THRESHOLD = 10.0;
#define FF_REFRESH_EVENT (SDL_USEREVENT)

struct AudioChannel {
    std::mutex mtx_;
    std::string uid;
    RingBuffer buffer_;

    explicit AudioChannel(const std::string& uid) : uid(uid), buffer_(uid, 0) {}

    void push(void* data, uint32_t size, double pts);
    void clear();
};

struct VideoChannel {
    struct PixelBuffer {
        void* data = nullptr;
        int pitch = 0;  // The number of bytes in a row of pixel data, including padding between lines.
        int capacity = 0;
        int w = 0;
        int h = 0;
        double pts = 0.0;
        PixelBuffer(void* _data, int _pitch, int _w, int _h, double _p);
        ~PixelBuffer();
        bool update(void* _data, int _pitch, int _w, int _h, double _p);
        using Ptr = std::unique_ptr<PixelBuffer>;
    };

    double frame_timer = static_cast<double>(av_gettime()) / 1000000.0;  // Sync fields
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

    bool ScreenSameIn(int w, int h) const;

    bool ScreenSameIn(const PixelBuffer::Ptr& pixel_buffer) const;

    bool ScreenVaild() const;

    void Destroy();

    bool Create(const PixelBuffer::Ptr& pixel_buffer);

    bool Create(int width, int height);

    void Update(AVFrame* data);

    void Update(const PixelBuffer::Ptr& pixel_buffer);

    void push(void* data, uint32_t size, int w, int h, double pts);
};

struct AudioContainer {
    std::set<std::unique_ptr<AudioChannel>> channels_;
    std::mutex mtx;
    static constexpr int cache_len = 1024 * 8;
    uint8_t cache[cache_len];

    void MixAudio(Uint8* stream, int len, MediaState* mediaState);

    void* add(const std::string& uid);

    void erase(void* that);
};

struct VideoContainer {
    std::mutex mtx;
    std::set<VideoChannel*> sources;
    std::map<Uint32, VideoChannel*> channels_;

    VideoChannel* add(const std::string& uid);
    void erase(VideoChannel* that);

    void* remove(Uint32 window_id);

    void clear();

    static uint32_t sdl_refresh_timer_cb(uint32_t interval, void* opaque);

    void schedule_refresh(int delay);

    void* find(Uint32 window_id);

    bool show_internal(VideoChannel* channel, struct MediaState& m);

    void clearBuffer(VideoChannel* channel);

    bool show(struct MediaState& m);

    void exit();
};

class SDLPlayer {
public:
    explicit SDLPlayer();
    virtual ~SDLPlayer() = default;

    SDL_AudioSpec audioSpec;
    AudioContainer audioContainer;
    VideoContainer videoContainer;
    MediaState* mediaState = nullptr;

    bool setMediaState(MediaState* m);

    static void AudioCallback(void* userdata, Uint8* stream, int len);

    bool running = true;

    void EventLoop(struct MediaState& m);

    void playAudio(int channels, int sample_rate, int nb_samples);

    void* openVideo(const std::string& uid, AVRegister::VideoPlayer* f);

    void pushVideoData(void* handle, void* data, uint32_t size, int w, int h, double pts);

    void closeVideo(void* handle);

    void* openAudio(const std::string& uid, AVRegister::PcmPlayer* f);

    void pushAudioData(void* handle, void* data, uint32_t size, double pts);

    void closeAudio(void* handle);

    void do_exit();
};
