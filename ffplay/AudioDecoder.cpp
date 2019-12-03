#include "AudioDecoder.h"
#include <cassert>
#include <iostream>

bool AACDecode::Decode(AVPacket *pkt, uint8_t *buf, int& len) {
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

        if (!pcm_convert) {
            pcm_convert = swr_alloc_set_opts(pcm_convert,
                                             av_get_default_channel_layout(channels), AV_SAMPLE_FMT_S16, sampleRate,
                                             av_get_default_channel_layout(channels), (AVSampleFormat)sampleFmt, sampleRate,
                                             0, nullptr);
            swr_init(pcm_convert);
        }
        uint64_t dst_nb_samples = av_rescale_rnd(swr_get_delay(pcm_convert, frame->sample_rate) + frame->nb_samples, frame->sample_rate, frame->sample_rate, AVRounding(1));

        int nb = swr_convert(pcm_convert, &buf, static_cast<int>(dst_nb_samples), (const uint8_t **)frame->data, frame->nb_samples);
        len = frame->channels * nb * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
        assert(nb > 0);
    }
    return true;
}