#include "SDLPlayer.h"

SDLPlayer::SDLPlayer() {
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);
}

// note: space：暂停 ->：快进10s <-：快退10s q/esc：退出 1/2/3：倍速播放
void AudioChannel::push(void* data, uint32_t size, double pts) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (buffer_.size() > 1024 * 1024 * 1024) {
        std::cout << "date send too quick" << std::endl;
        abort();
    }
    buffer_.write(data, size);
}

void AudioChannel::clear() {
    std::lock_guard<std::mutex> lock(mtx_);
    buffer_.clean();
}

bool VideoChannel::ScreenSameIn(int w, int h) const {
    return std::abs(rect.w - w) < 10 && std::abs(rect.h - h) < 10;
}

bool VideoChannel::ScreenSameIn(const VideoChannel::PixelBuffer::Ptr& pixel_buffer) const {
    return std::abs(rect.w - pixel_buffer->w) < 10 && std::abs(rect.h - pixel_buffer->h) < 10;
}

bool VideoChannel::ScreenVaild() const {
    return rect.w != 0 && rect.h != 0;
}

void VideoChannel::Destroy() {
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(texture);
    window = nullptr;
    renderer = nullptr;
    texture = nullptr;
    rect.w = 0;
    rect.h = 0;
}

bool VideoChannel::Create(const VideoChannel::PixelBuffer::Ptr& pixel_buffer) {
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

bool VideoChannel::Create(int width, int height) {
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

void VideoChannel::Update(AVFrame* data) {
    SDL_UpdateYUVTexture(texture, nullptr, data->data[0], data->linesize[0],
                         data->data[1], data->linesize[1],
                         data->data[2], data->linesize[2]);
    SDL_RenderCopy(renderer, texture, nullptr, &rect);
    SDL_RenderPresent(renderer);
}

void VideoChannel::Update(const VideoChannel::PixelBuffer::Ptr& pixel_buffer) {
    assert(ScreenSameIn(pixel_buffer));
    SDL_UpdateTexture(texture, nullptr, pixel_buffer->data, pixel_buffer->pitch);
    SDL_RenderCopy(renderer, texture, nullptr, &rect);
    SDL_RenderPresent(renderer);
}

void VideoChannel::push(void* data, uint32_t size, int w, int h, double pts) {
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

void AudioContainer::MixAudio(Uint8* stream, int len, MediaState* mediaState) {
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
            mediaState->audioState.audio_buff_consume += len1;
        }
    }
}

void* AudioContainer::add(const std::string& uid) {
    std::lock_guard<std::mutex> lock(mtx);
    auto a = std::make_unique<AudioChannel>(uid);
    auto r = channels_.insert(std::move(a));
    return r.second ? r.first->get() : nullptr;
}

void AudioContainer::erase(void* that) {
    std::lock_guard<std::mutex> lock(mtx);
    auto iter = std::find_if(channels_.begin(), channels_.end(),
                             [&that](const std::unique_ptr<AudioChannel>& x) -> bool {
                                 return static_cast<void*>(x.get()) == that;
                             });
    channels_.erase(iter);
}

VideoChannel* VideoContainer::add(const std::string& uid) {
    auto item = new VideoChannel(uid);
    std::lock_guard<std::mutex> lock(mtx);
    sources.insert(item);
    return item;
}

void VideoContainer::erase(VideoChannel* that) {
    std::lock_guard<std::mutex> lock(mtx);
    sources.erase(that);
}

void* VideoContainer::remove(Uint32 window_id) {
    auto iter = channels_.find(window_id);
    void* p = nullptr;
    if (iter != channels_.end()) {
        p = iter->second;
        channels_.erase(iter);
    }
    return p;
}

void VideoContainer::clear() {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& iter : sources) {
        clearBuffer(iter);
    }
    sources.clear();
    channels_.clear();
}

uint32_t VideoContainer::sdl_refresh_timer_cb(uint32_t interval, void* opaque) {
    SDL_Event event;
    event.type = FF_REFRESH_EVENT;
    SDL_PushEvent(&event);
    return 0;
}

void VideoContainer::schedule_refresh(int delay) {
    SDL_AddTimer(delay, sdl_refresh_timer_cb, nullptr);
}

void* VideoContainer::find(Uint32 window_id) {
    auto iter = channels_.find(window_id);
    if (iter == channels_.end()) {
        return nullptr;
    }
    SDL_GetWindowSize(iter->second->window, &iter->second->rect.w, &iter->second->rect.h);
    return iter->second;
}

VideoChannel::PixelBuffer::PixelBuffer(void* _data, int _pitch, int _w, int _h, double _p) {
    capacity = _w * _h * 3 / 2 + 1;
    data = new uint8_t[capacity];
    update(_data, _pitch, _w, _h, _p);
}

VideoChannel::PixelBuffer::~PixelBuffer() {
    if (data) {
        delete[] static_cast<uint8_t*>(data);
    }
}

bool VideoChannel::PixelBuffer::update(void* _data, int _pitch, int _w, int _h, double _p) {
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

bool VideoContainer::show_internal(VideoChannel* channel, struct MediaState& m) {
    std::unique_lock<std::mutex> lock_(channel->mtx_);
    if (channel->work_queue_.empty()) {
        schedule_refresh(1);
        return false;
    }
    auto pixel_buffer = std::move(channel->work_queue_.front());
    channel->work_queue_.pop();
    lock_.unlock();

    auto current_pts = pixel_buffer->pts;
    auto delay = current_pts - channel->frame_last_pts;
    if (delay <= 0 || delay >= 1.0)
        delay = channel->frame_last_delay;

    channel->frame_last_delay = delay;
    channel->frame_last_pts = current_pts;

    // 当前显示帧的PTS来计算显示下一帧的延迟
    auto ref_clock = m.audioState.get_audio_clock();

    auto diff = current_pts - ref_clock;  // diff < 0 => video slow,diff > 0 => video quick

    auto threshold = (delay > SYNC_THRESHOLD) ? delay : SYNC_THRESHOLD;

    if (fabs(diff) < NOSYNC_THRESHOLD) {
        if (diff <= -threshold) {
            // 慢了，delay设为0
            delay = 0;
        } else if (diff >= threshold) {
            // 快了，加倍delay
            delay *= 2;
        }
    }

    channel->frame_timer += delay;
    double actual_delay = channel->frame_timer - static_cast<double>(av_gettime()) / 1000000.0;
    if (actual_delay <= 0.010)
        actual_delay = 0.010;

    //        std::cout << "delay time : " << static_cast<int>(actual_delay * 1000 + 0.5) << std::endl;
    schedule_refresh(static_cast<int>(actual_delay * 1000 + 0.5));

    if (channel->ScreenVaild() && !channel->ScreenSameIn(pixel_buffer)) {
        channel->Destroy();
    }
    if (!channel->ScreenVaild()) {
        if (!channel->Create(pixel_buffer)) {
            return false;
        }
        channels_.emplace(channel->window_id, channel);
    }

    channel->Update(pixel_buffer);
    lock_.lock();
    channel->ready_queue_.push(std::move(pixel_buffer));
    return true;
}

void VideoContainer::clearBuffer(VideoChannel* channel) {
    std::unique_lock<std::mutex> lock_(channel->mtx_);
    while (channel->work_queue_.empty()) {
        channel->work_queue_.pop();
    }
    while (channel->ready_queue_.empty()) {
        channel->ready_queue_.pop();
    }
    channel->Destroy();
}

bool VideoContainer::show(struct MediaState& m) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& v : sources) {
        show_internal(v, m);
    }
    return true;
}

void VideoContainer::exit() {
    clear();
}

bool SDLPlayer::setMediaState(MediaState* m) {
    if (!m) {
        return false;
    } else {
        mediaState = m;
        return true;
    }
}

void SDLPlayer::AudioCallback(void* userdata, Uint8* stream, int len) {
    auto that = (SDLPlayer*)userdata;
    that->audioContainer.MixAudio(stream, len, that->mediaState);
}

void SDLPlayer::EventLoop(struct MediaState& m) {
    SDL_Event event;
    while (running) {
        SDL_WaitEvent(&event);
        switch (event.type) {
            case FF_REFRESH_EVENT:
                videoContainer.show(m);
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_SPACE:
                        break;
                    case SDLK_LEFT:
                        break;
                    case SDLK_RIGHT:
                        break;
                    case SDLK_q:
                    case SDLK_ESCAPE:
                        m.exit();
                        do_exit();
                        break;
                    default:
                        break;
                }
            case SDL_MOUSEBUTTONDOWN:
                break;
            case SDLK_SPACE:
            default:
                break;
        }
    }
    printf("Exit %s \n", __FUNCTION__);
}

void SDLPlayer::playAudio(int channels, int sample_rate, int nb_samples) {
    if (audioSpec.channels == channels && audioSpec.freq == sample_rate && audioSpec.samples == nb_samples) {
        return;
    }
    std::cout << "nb_sample from ffmpeg : " << nb_samples << std::endl;
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

void* SDLPlayer::openVideo(const std::string& uid, AVRegister::VideoPlayer* f) {
    using namespace std::placeholders;
    *f = std::bind(&SDLPlayer::pushVideoData, this, _1, _2, _3, _4, _5, _6);
    return videoContainer.add(uid);
}

void SDLPlayer::pushVideoData(void* handle, void* data, uint32_t size, int w, int h, double pts) {
    assert(handle);
    auto that = static_cast<VideoChannel*>(handle);
    that->push(data, size, w, h, pts);
}

void SDLPlayer::closeVideo(void* handle) {
    assert(handle);
    auto that = static_cast<VideoChannel*>(handle);
    videoContainer.erase(that);
    if (that->ScreenVaild()) {
        that->Destroy();
    }
    videoContainer.remove(that->window_id);
    delete that;
}

void* SDLPlayer::openAudio(const std::string& uid, AVRegister::PcmPlayer* f) {
    using namespace std::placeholders;
    *f = std::bind(&SDLPlayer::pushAudioData, this, _1, _2, _3, _4);
    return audioContainer.add(uid);
}

void SDLPlayer::pushAudioData(void* handle, void* data, uint32_t size, double pts) {
    assert(handle);
    auto that = static_cast<AudioChannel*>(handle);
    that->push(data, size, pts);
}

void SDLPlayer::closeAudio(void* handle) {
    assert(handle);
    audioContainer.erase(handle);
}

void SDLPlayer::do_exit() {
    videoContainer.exit();
    SDL_CloseAudio();
    SDL_Quit();
    running = false;
}
