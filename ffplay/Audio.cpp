
#include "Audio.h"

#include <fstream>
#include <iostream>
extern "C" {

#include <libswresample/swresample.h>
}

AudioState::AudioState()
    : BUFFER_SIZE(192000) {
    stream = nullptr;
    audio_clock = 0;

    audio_buff = new uint8_t[BUFFER_SIZE];
    audio_buff_size = 0;
    audio_buff_index = 0;
}

AudioState::~AudioState() {
    if (audio_buff)
        delete[] audio_buff;
}

bool AudioState::audio_play() {
    while (!aacDecode.codecCtx_)
        ;
    SDL_AudioSpec desired;
    desired.freq = aacDecode.codecCtx_->sample_rate;
    desired.channels = aacDecode.codecCtx_->channels;
    desired.format = AUDIO_S16SYS;
    desired.samples = 1024;
    desired.silence = 0;
    desired.userdata = this;
    desired.callback = audio_callback;

    if (SDL_OpenAudio(&desired, nullptr) < 0) {
        return false;
    }

    SDL_PauseAudio(0);  // playing

    return true;
}

double AudioState::get_audio_clock() {
    int hw_buf_size = audio_buff_size - audio_buff_index;
    int bytes_per_sec = stream->codecpar->sample_rate * aacDecode.codecCtx_->channels * 2;

    double pts = audio_clock - static_cast<double>(hw_buf_size) / bytes_per_sec;

    return pts;
}

/**
* 向设备发送audio数据的回调函数
*/
void audio_callback(void *userdata, Uint8 *stream, int len) {
    AudioState *audio_state = (AudioState *)userdata;

    SDL_memset(stream, 0, len);

    int audio_size = 0;
    int len1 = 0;
    while (len > 0)  // 向设备发送长度为len的数据
    {
        if (audio_state->audio_buff_index >= audio_state->audio_buff_size)  // 缓冲区中无数据
        {
            // 从packet中解码数据
            audio_size = audio_decode_frame(audio_state, audio_state->audio_buff, sizeof(audio_state->audio_buff));
            if (audio_size < 0)  // 没有解码到数据或出错，填充0
            {
                audio_state->audio_buff_size = 0;
                memset(audio_state->audio_buff, 0, audio_state->audio_buff_size);
            } else
                audio_state->audio_buff_size = audio_size;

            audio_state->audio_buff_index = 0;
        }
        len1 = audio_state->audio_buff_size - audio_state->audio_buff_index;  // 缓冲区中剩下的数据长度
        if (len1 > len)                                                       // 向设备发送的数据长度为len
            len1 = len;

        SDL_MixAudio(stream, audio_state->audio_buff + audio_state->audio_buff_index, len, SDL_MIX_MAXVOLUME);

        len -= len1;
        stream += len1;
        audio_state->audio_buff_index += len1;
    }
}

int audio_decode_frame(AudioState *audio_state, uint8_t *audio_buf, int buf_size) {
    int data_size = 0;
    AVPacket pkt;

    if (!audio_state->audioq.deQueue(&pkt, true))
        return -1;

    if (pkt.pts != AV_NOPTS_VALUE) {
        audio_state->audio_clock = av_q2d(audio_state->stream->time_base) * pkt.pts;
    }

    if (audio_state->aacDecode.Decode(&pkt, audio_buf, data_size)) {
        // 每秒钟音频播放的字节数 sample_rate * channels * sample_format(一个sample占用的字节数)
        audio_state->audio_clock += static_cast<double>(data_size) / (2 * audio_state->stream->codecpar->channels * audio_state->stream->codecpar->sample_rate);
    }

    return data_size;
}
