#pragma once

#include <cstdint>
#include <functional>
#include <mutex>

#include "H264Decoder.h"
#include "AudioDecoder.h"

namespace duobei {

namespace decoder_ {

struct Decoder {
    using FromVideo = std::function<int(uint8_t*, uint32_t, uint32_t)>;
    using FromAudio = std::function<int(uint16_t *, uint32_t, bool, uint32_t)>;

    FromVideo from_video;
    FromAudio from_audio;

    void setCallback(FromVideo v) {
        from_video = std::move(v);
    }

    void setCallback(FromAudio a) {
        from_audio = std::move(a);
    }

    int Audio(uint16_t * data, uint32_t size, bool raw, uint32_t timestamp) {
        if (from_audio == nullptr) {
            return 0;
        }
        return from_audio(data, size, raw, timestamp);
    }

    int Video(uint8_t* data, uint32_t size, uint32_t timestamp) {
        if (from_video == nullptr) {
            return -1;
        }
        return from_video(data, size, timestamp);
    }
};

}

namespace parse_ {

struct CacheSpan {
    std::mutex audio_binding_mtx_;
    audio::AudioDecoder audioDecoder;

    std::mutex video_binding_mtx_;
    video::H264Decoder videoDecoder;
};

struct DecoderSpan : public CacheSpan {
    decoder_::Decoder decoder_;

    std::string uid;

    void setUid(const std::string& uid_) {
        uid = uid_;
        using namespace std::placeholders;
        decoder_::Decoder::FromVideo video = std::bind(&video::H264Decoder::Decode, &videoDecoder, _1, _2, _3);
        decoder_.setCallback(video);

        decoder_::Decoder::FromAudio audio = std::bind(&audio::AudioDecoder::Decode, &audioDecoder, _1, _2, _3, _4);
        decoder_.setCallback(audio);
    }

    bool BindVideoCallbackPlaying() {
        std::lock_guard lock(video_binding_mtx_);
        if (videoDecoder.play_internal.handle) {
            return false;
        }
        return videoDecoder.Init(uid);
    }

    bool ResetVideoCallbackPlaying() {
        std::lock_guard lock(video_binding_mtx_);
        if (!videoDecoder.play_internal.handle) {
            return false;
        }
        if (videoDecoder.play_internal.handle) {
            return videoDecoder.Destroy();
        }
    }

    bool BindAudioCallbackPlaying() {
        std::lock_guard lock(audio_binding_mtx_);
        if (audioDecoder.play_internal.handle) {
            return false;
        }
        return audioDecoder.Init(uid);
    }

    bool ResetAudioCallbackPlaying() {
        std::lock_guard lock(audio_binding_mtx_);
        if (!audioDecoder.play_internal.handle) {
            return false;
        }
        return audioDecoder.Destroy();
    }

    using InvokeVideoType = int (DecoderSpan::*)(uint8_t* data, uint32_t size, uint32_t timestamp);
    using InvokeAudioType = int (DecoderSpan::*)(uint16_t * data, uint32_t size, bool raw, uint32_t timestamp);

    int Video(uint8_t* data, uint32_t size, uint32_t timestamp) {
        return decoder_.Video(data, size, timestamp);
    }

    int Audio(uint16_t * data, uint32_t size, bool raw, uint32_t timestamp) {
        return decoder_.Audio(data, size, raw, timestamp);
    }

    void Init(std::string &uid) {
        setUid(uid);
        BindVideoCallbackPlaying();
        BindAudioCallbackPlaying();
    }
};

}
}
