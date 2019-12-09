//
// Created by Johnny on 2/8/18.
//

#pragma once


#include "../utils/Time.h"

#include <cassert>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

struct AVFormatContext;
struct AVCodecParameters;
struct AVCodecContext;
struct AVCodec;
struct SwsContext;
struct AVFrame;
struct AVPacket;
struct AVIOContext;

namespace duobei {
namespace format {
struct Element {
    uint8_t* video = nullptr;
    int8_t* audio = nullptr;
    int length = 0;
    int capacity = 0;
    int width = 0;
    int height = 0;
    int format = 0;
    bool key = false;
    uint32_t timestamp = 0;
    bool empty = true;

    // YUV
    Element(const uint8_t* buffer, int size, int width_, int height_, int format_, uint32_t timestamp_) noexcept {
        //capacity = utility::nextPowerOf2(size);
        capacity = size + 1;
        video = new uint8_t[capacity];
        update(width_, height_, format_);
        [[maybe_unused]] auto r = update(buffer, size, false, timestamp_);
        assert(r);
    }
    // H264
    Element(const uint8_t* buffer, int length_, bool keyFrame, uint32_t timestamp_) noexcept {
        //capacity = utility::nextPowerOf2(length_);
        capacity = length_ + 1;
        video = new uint8_t[capacity];
        [[maybe_unused]] auto r = update(buffer, length_, keyFrame, timestamp_);
        assert(r);
    }

    // Audio
    Element(const int8_t* buffer, int length_, uint32_t timestamp_) noexcept {
        //capacity = utility::nextPowerOf2(length_);
        capacity = length_ + 1;
        audio = new int8_t[capacity];
        [[maybe_unused]] auto r = update(buffer, length_, timestamp_);
        assert(r);
    }

    bool isAudio() const { return video == nullptr; }

    ~Element() {
        if (isAudio()) {
            delete[] audio;
        } else {
            delete[] video;
        }
    }

    void update(int width_, int height_, int format_) {
        width = width_;
        height = height_;
        format = format_;
    }
    bool update(const int8_t* buffer, int size, uint32_t ts) {
        if (size > capacity) {
            return false;
        }

        length = size;
        timestamp = ts;
        if (buffer) {
            memcpy(audio, buffer, length);
            empty = false;
        } else {
            empty = true;
        }
        return true;
    }

    bool update(const uint8_t* buffer, int size, bool k, uint32_t ts) {
        if (size > capacity) {
            return false;
        }
        length = size;
        timestamp = ts;
        key = k;
        if (buffer) {
            memcpy(video, buffer, length);
            empty = false;
        } else {
            empty = true;
        }
        return true;
    }

    using Ptr = std::unique_ptr<Element>;
    using Container = std::vector<Ptr>;

    struct Compare {
        bool operator()(const Ptr& a, const Ptr& b) {
            return a->length > b->length;
        }
    };
    using PriorityQueue = std::priority_queue<Ptr, Container, Compare>;

    struct TimeSequenceCompare {
        bool operator()(const Ptr& a, const Ptr& b) {
            return a->timestamp > b->timestamp;
        }
    };
    using TimeSequenceQueue = std::priority_queue<Ptr, Container, TimeSequenceCompare>;

    using Queue = std::queue<Ptr>;

    // note: 就绪队列入队
    static void PushReadyPriorityQueue(PriorityQueue& queue_, Ptr&& e) {
        if (queue_.empty()) {
            queue_.push(std::move(e));
        } else {
            auto& t = queue_.top();
            if (e->capacity > t->capacity) {
                queue_.pop();
                queue_.push(std::move(e));
            }
        }
    }

    static void Clear(Queue& queue) {
        while (!queue.empty()) {
            queue.pop();
        }
    }

    template <typename... _Args>
    static Ptr New(_Args&&... __args) {
        return std::make_unique<Element>(std::forward<_Args>(__args)...);
    }
};
}

namespace Video {

bool isSPS(const uint8_t* buf, int& len);

bool isPPS(const uint8_t* buf, int& len);

bool isNAL(const uint8_t* buf, int& len);

bool isIFM(const uint8_t* buf, int& len);

namespace detail {
class VideoBuffer {
    size_t capacity_ = 0;

public:
    uint8_t* buf = nullptr;
    int size = 0;
    bool isKeyFrame = false;
    uint32_t timestamp = 0;

    bool isH264 = false;  //是否已经编码成h264

    struct VideoSize {
        int Width = 0;
        int Height = 0;
        int Type = 0;
        constexpr int Size() const { return Width * Height; }
        constexpr int Length() const {
            return Size() * 3 / 2;
        }  // YUV420P
        constexpr bool Valid() const { return Height > 0 && Width > 0; }
        void Set(int w, int h, int t) {
            Width = w;
            Height = h;
            Type = t;
        }
    };
    VideoSize src;
    VideoSize dst;

    void resetBuffer() {
        if (capacity_ > 0) {
            delete[] buf;
        }
    }

    void allocBuffer(size_t length) {
        capacity_ = length;
        buf = new uint8_t[capacity_];
    }

    void setBuffer(const uint8_t* buffer, int len) {
        size = len;
        auto length = static_cast<size_t>(len);
        if (capacity_ < length) {
            resetBuffer();
            allocBuffer(length);
        }
        memcpy(buf, buffer, length);
    }

    explicit VideoBuffer(const uint8_t* videoBuffer, int len, bool key, uint32_t ts) {
        isH264 = true;
        isKeyFrame = key;
        timestamp = ts;
        setBuffer(videoBuffer, len);
    }

    explicit VideoBuffer(const uint8_t* videoBuffer, int w, int h, int type) {
        src.Set(w, h, type);
        int l = src.Length();
        if (l <= 0) {
            size = 0;
            return;
        }
        isH264 = false;
        setBuffer(videoBuffer, l);
    }

    void setTimestamp(uint32_t t) {
        timestamp = t;
    }

    ~VideoBuffer() {
        resetBuffer();
    }
};
using VideoList = std::list<std::shared_ptr<VideoBuffer>>;
};  // namespace detail
using namespace detail;
class VideoSender {
    VideoList outdatedList_;
    VideoList sendingList_;
    std::mutex videoMutex_;

    std::shared_ptr<VideoBuffer> getVideoBuffer(const uint8_t* videoBuffer, int len, bool isKeyframe, uint32_t ts) {
        if (outdatedList_.empty()) {
            return std::make_shared<VideoBuffer>(videoBuffer, len, isKeyframe, ts);
        }
        auto video = outdatedList_.front();
        if (video.use_count() > 2) {
            outdatedList_.pop_front();
        }

        // devType 忽略
        video->setBuffer(videoBuffer, len);
        video->isKeyFrame = isKeyframe;

        return video;
    }

    std::shared_ptr<VideoBuffer> getVideoBuffer(const uint8_t* videoBuffer, int width, int height, int devType) {
        if (outdatedList_.empty()) {
            return std::make_shared<VideoBuffer>(videoBuffer, width, height, devType);
        }

        // 网络重连时会影响这里
        auto video = outdatedList_.front();
        if (video.use_count() > 2) {
            outdatedList_.pop_front();
        }
        //WriteDebugLog("outdatedList_ %u use_count %ld", outdatedList_.size(), video.use_count());
        video->src.Set(width, height, devType);
        auto len = video->src.Length();

        video->setBuffer(videoBuffer, len);
        return video;
    }

public:
    VideoSender() = default;
    ~VideoSender() = default;

    void pushH264Buffer(const uint8_t* videoBuffer, int len, bool isKeyframe, uint32_t ts) {
	    std::lock_guard<std::mutex> lck(videoMutex_);
        auto video = getVideoBuffer(videoBuffer, len, isKeyframe, ts);
        //WriteDebugLog("sendingList_ size %lu", sendingList_.size());
        //最多缓存10帧
        if (sendingList_.size() > 20) {
            sendingList_.clear();
        }
        sendingList_.push_back(video);
    }

    bool pushYuvBuffer(const uint8_t* videoBuffer, int dstWidth, int dstHeight, int devType, int srcWidth, int srcHeight) {
        std::lock_guard<std::mutex> lck(videoMutex_);
        auto video = getVideoBuffer(videoBuffer, srcWidth, srcHeight, devType);
        video->dst.Set(dstWidth, dstHeight, devType);
        if (video->size == 0) {
            return false;
        }

        //最多缓存20帧
        if (sendingList_.size() > 20) {
            sendingList_.clear();
        }
        sendingList_.push_back(video);
        return true;
    }

    std::shared_ptr<VideoBuffer> popVideoBuffer() {
        std::lock_guard<std::mutex> lck(videoMutex_);
        if (sendingList_.empty()) {
            return nullptr;
        }
        auto video = sendingList_.front();
        sendingList_.pop_front();
        outdatedList_.push_back(video);
        //WriteDebugLog("sendingList_ %u use_count %ld, length = %ld", sendingList_.size(), video.use_count());
        return video;
    }

    void clearVideoBuffer() {
        std::lock_guard<std::mutex> lck(videoMutex_);
        sendingList_.clear();
    }
};

class VideoData {
    int ptsCount = 0;
    bool changed = false;
    std::vector<uint8_t> buffer;

public:
    int Width = 0;
    int Height = 0;
    AVFrame* Frame = nullptr;

    bool resolutionChange(int w, int h) {
        changed = false;
        if (Width == 0 && Height == 0) {
            Width = w;
            Height = h;
        } else if (Width != w || Height != h) {
            Width = w;
            Height = h;
            changed = true;
        }
        return changed;
    }
    bool Changed() {
        bool tmp = changed;
        changed = false;
        return tmp;
    }
    bool updateFrame(int width, int height);
    void fillFrame(const uint8_t* src, int pixelFormat);
    void fillFrame(int pixelFormat);
    void resetFrame();
    void setCodecOptions(const AVCodecContext* ctx);
    void setFrameOptions(int format);

    explicit VideoData() {
        allocFrame();
    }
    ~VideoData() {
        resetFrame();
    }

    void allocFrame();
};

class CodecContext {
    bool initCodec = false;
    AVCodec* codec = nullptr;

public:
    CodecContext() = default;
    ~CodecContext() {
        Close();
    }
    int PacketSize();
    AVCodecContext* ctx = nullptr;
    bool Opened() {
        return initCodec;
    }
    void Close();
    bool SetCodec(int width, int height);
    bool OpenCodec();
};

class VideoContext {
public:
    CodecContext codec;

    explicit VideoContext() = default;

    ~VideoContext() {
        resetContext();
    }

    void resetContext() {
        codec.Close();
    }
};

class VideoConversion {
    int pixelType = 0;

    struct SwsContext* img_convert = nullptr;

    void resetFrameContext();

public:
    VideoData src;
    VideoData dst;

    void fillBuffer(const uint8_t* videoBuffer, int srcWidth, int srcHeight, int devType, int dstWidth, int dstHeight);

    bool convertFrame(int devType);

    void resetSwsContext();

    bool resolutionChange() {
        return src.Changed() || dst.Changed();
    }

    void setCodecOptions(int format) {
        dst.setFrameOptions(format);
        src.setFrameOptions(format);
        //        dst.setCodecOptions(codectx.codec.ctx);
    }
};

}  // namespace Video
}  // namespace duobei
