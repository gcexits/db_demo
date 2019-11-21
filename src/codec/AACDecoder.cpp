#include "AACDecoder.h"
#include <cassert>

bool AudioDecode::Decode(AVPacket *pkt) {
    int result = avcodec_send_packet(codecCtx_, pkt);
    if (result < 0) {
        return false;
    }
    while (result == 0) {
        result = avcodec_receive_frame(codecCtx_, frame);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
            break;
        }
        if (result < 0) {
            return false;
        }
        if (!isInited()) {
            channels = frame->channels;
            sampleFmt = frame->format;
            sampleRate = frame->sample_rate;
            nb_samples = frame->nb_samples;
        }
        int buffersize = av_samples_get_buffer_size(nullptr, frame->channels, frame->nb_samples, AV_SAMPLE_FMT_S16, 1);
        uint8_t *dstPcm = new uint8_t[buffersize + 1];
        AVFrame *dstFram = av_frame_alloc();
        {
            dstFram->channels = channels;
            dstFram->channel_layout = av_get_default_channel_layout(channels);
            dstFram->format = 1;
            dstFram->sample_rate = sampleRate;
            auto nb_samples_ = static_cast<double>(frame->nb_samples * dstFram->sample_rate) / static_cast<double>(codecCtx_->sample_rate) + 0.5;
            dstFram->nb_samples = static_cast<int>(nb_samples_);
        }
        int ret = avcodec_fill_audio_frame(dstFram, frame->channels, AV_SAMPLE_FMT_S16, dstPcm, buffersize, 1);
        assert(ret > 0);
        if (!pcm_convert) {
            pcm_convert = swr_alloc_set_opts(pcm_convert,
                                             av_get_default_channel_layout(channels), AV_SAMPLE_FMT_S16, sampleRate,
                                             av_get_default_channel_layout(channels), (AVSampleFormat)sampleFmt, sampleRate,
                                             0, nullptr);
            swr_init(pcm_convert);
        }
        ret = swr_convert(pcm_convert, dstFram->data, dstFram->nb_samples, (const uint8_t **)frame->data, frame->nb_samples);
        assert(ret > 0);
        playInternal.Play(dstFram->data[0], buffersize);
        delete[] dstPcm;
        av_frame_free(&dstFram);
        //        av_fast_malloc
    }
    return true;
}