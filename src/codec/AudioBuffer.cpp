#include "AudioBuffer.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

#include "../utils/Optional.h"

namespace duobei {

enum AVSampleFormat getSampleFormat(int devType) {
    enum AVSampleFormat formats[] = {
        // AV_SAMPLE_FMT_NONE = -1,
        AV_SAMPLE_FMT_U8,
        AV_SAMPLE_FMT_S16,
        AV_SAMPLE_FMT_S32,
        AV_SAMPLE_FMT_FLT,
        AV_SAMPLE_FMT_DBL,

        AV_SAMPLE_FMT_U8P,
        AV_SAMPLE_FMT_S16P,
        AV_SAMPLE_FMT_S32P,
        AV_SAMPLE_FMT_FLTP,
        AV_SAMPLE_FMT_DBLP,
        // todo:如果打开注释，目前android ndk-build编译时会失败
        // AV_SAMPLE_FMT_S64,
        // AV_SAMPLE_FMT_S64P,

        AV_SAMPLE_FMT_NB};
    size_t formatIndex = static_cast<size_t>(devType);
    if (devType < 0 || formatIndex >= sizeof(formats) / sizeof(*formats)) {
        formatIndex = 0;
    }
    return formats[formatIndex];
}

namespace audio {

int AudioOption::nb_samples() const {
    return sampleRate / 50;
}

void AudioData::updateFrame(const AudioOption& o) {
    samplingChange(o.channels, o.sampleRate, o.sampleFmt);
}

void AudioData::updateFrame(const CodecContext& o) {
    samplingChange(o.channels, o.sampleRate, o.sampleFmt);
}

void AudioData::updateFrame(const AVCodecContext* c) {
    samplingChange(c->channels, c->sample_rate, c->sample_fmt);
}

void AudioData::fillFrame(const uint8_t* audioBuffer, int bufferSize) {
    avcodec_fill_audio_frame(Frame, channels, (AVSampleFormat)sampleFmt, audioBuffer, bufferSize, 0);
}

void AudioData::fillFrame() {
    bufferSize = av_samples_get_buffer_size(nullptr, channels, Frame->nb_samples, (AVSampleFormat)sampleFmt, 0);
    if (buffer.capacity() < static_cast<size_t>(bufferSize)) {
        buffer.resize(bufferSize, '\0');
    }
    fillFrame(buffer.data(), bufferSize);
}

void AudioData::resetFrame() {
    if (Frame) {
        av_frame_free(&Frame);
        Frame = nullptr;
    }
}

int AudioData::BufferSize() {
    return av_samples_get_buffer_size(nullptr, channels, Frame->nb_samples, (AVSampleFormat)sampleFmt, 0);
}

void AudioData::setCodecOptions(int nb_samples) {
    Frame->channels = 1;
    Frame->channel_layout = av_get_default_channel_layout(1);
    Frame->format = 1;
    Frame->sample_rate = sampleRate;
    Frame->nb_samples = nb_samples;
}

void AudioData::allocFrame() {
    if (!Frame) {
        Frame = av_frame_alloc();
    }
}

bool CodecContext::SetCodec(const AVCodecParameters* param) {
    decodeCtx = avcodec_alloc_context3(nullptr);
    if (!decodeCtx) {
        return false;
    }

    if (auto ret = avcodec_parameters_to_context(decodeCtx, param); ret != 0) {
        avcodec_free_context(&decodeCtx);
        return false;
    }
    decoder = avcodec_find_decoder(decodeCtx->codec_id);
    if (!decoder) {
        avcodec_free_context(&decodeCtx);
        decodeCtx = nullptr;
        return false;
    }
    return true;
}

bool CodecContext::OpenCodec() {
    assert(decoder);
    if (auto ret = avcodec_open2(decodeCtx, decoder, nullptr); ret < 0) {
        avcodec_free_context(&decodeCtx);
        decodeCtx = nullptr;
        return false;
    }
    initCodec = true;
    return true;
}

void CodecContext::Close() {
    if (Opened()) {
        if (decodeCtx) {
            avcodec_close(decodeCtx);
            avcodec_free_context(&decodeCtx);
            decodeCtx = nullptr;
        }
        initCodec = false;
    }
}

void AudioSampling::reset() {
    if (pcm_convert) {
        swr_free(&pcm_convert);
        pcm_convert = nullptr;
    }
}

bool AudioSampling::convert() {
    if (!pcm_convert) {
        pcm_convert = swr_alloc_set_opts(pcm_convert,
                                         av_get_default_channel_layout(dst.channels), (AVSampleFormat)dst.sampleFmt, dst.sampleRate,
                                         av_get_default_channel_layout(src.channels), (AVSampleFormat)src.sampleFmt, src.sampleRate,
                                         0, nullptr);

        if (!pcm_convert) {
            return false;
        }
        swr_init(pcm_convert);
    }

    int ret = swr_convert(pcm_convert, dst.Frame->data, dst.Frame->nb_samples, (const uint8_t**)src.Frame->data, src.Frame->nb_samples);

    return ret != 0;
}

void AudioSampling::setDstFrameOptions() {
    dst.setCodecOptions(320);
}

void AudioSampling::resetFrameContext() {
    src.resetFrame();
    dst.resetFrame();
}

void AudioSampling::fillBuffer() {
    dst.fillFrame();
}

void AudioContext::setSrcFrameOptions() {
    //int srcChannels = codec.decodeCtx->channels;
    //int srcSampleRate = codec.decodeCtx->sample_rate;
    //sampling.src.updateFrame(srcChannels, srcSampleRate, codec.decodeCtx);
    sampling.src.updateFrame(codec.decodeCtx);
}

void AudioContext::setDstFrameOptions() {
    //int dstChannels = codec.channels;
    //int dstSampleRate = codec.sampleRate;
    //auto dstFormat = getSampleFormat(codec.sampleFmt);
    //WriteDebugLog("codec.sampleFmt=%d, AV_SAMPLE_FMT_S16=%d", codec.sampleFmt, AV_SAMPLE_FMT_S16);
    //sampling.dst.updateFrame(dstChannels, dstSampleRate, codec.sampleFmt);
    sampling.dst.updateFrame(codec);
}

void AudioSamplingTest() {
    struct AudioSampling sampling;

    while (!sampling.DataInit()) {
        AudioOption src(1, 48000, 1);
        AudioOption dst(1, 16000, 1);

        sampling.src.updateFrame(src);
        sampling.dst.setCodecOptions(320);
        sampling.dst.updateFrame(dst);
        sampling.dst.fillFrame();
        uint8_t buffer[2048];
        sampling.src.fillFrame(buffer, 2048);

        sampling.convert();
        //use sampling.dst.Frame->data[0], sampling.dst.Frame->nb_samples * 2
    }
}

}  // namespace audio
}  // namespace duobei
