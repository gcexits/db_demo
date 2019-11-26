#include "SpeexEncoder.h"
#include "../utils/Optional.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

namespace duobei {
namespace audio {
bool SpeexEncoderContext::Init() {
    audio_buffer_[0] = 0xB2;
#if defined(kMobilePlatform)
    int quality = 6;
    int sample_rate, vad = 1, denoise = 1, noiseSuppress = -21;

    speex_bits_init(&enc_bits);
    enc_state = speex_encoder_init(&speex_wb_mode);
    speex_encoder_ctl(enc_state, SPEEX_SET_QUALITY, &quality);
    int tmp = 2;
    speex_encoder_ctl(enc_state, SPEEX_SET_COMPLEXITY, &tmp);

    speex_encoder_ctl(enc_state, SPEEX_GET_FRAME_SIZE, &enc_frame_size);
    speex_encoder_ctl(enc_state, SPEEX_GET_SAMPLING_RATE, &sample_rate);
    //回声消除模块
    /* Echo canceller with 200 ms tail length */
    // echo_state = speex_echo_state_init(frame_size, 10 * frame_size);
    // tmp = samplerate;
    // speex_echo_ctl(echo_state, SPEEX_ECHO_SET_SAMPLING_RATE, &tmp);

    // /* Setup preprocessor and associate with echo canceller for residual echo suppression */
    // preprocess = speex_preprocess_state_init(frame_size, samplerate);
    // speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_ECHO_STATE, echo_state);
    // speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_DENOISE, &denoise);               //降噪
    // speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noiseSuppress);  //设置噪声的dB

    // speex_encoder_ctl(enc_state, SPEEX_SET_VAD, &vad);
    pkt_frame_count = 0;
#endif

#if defined(kDesktopPlatform)
    int quality = 8;  // speex 编码质量, 原来的值为 6
    int sample_rate, vad = 1, denoise = 1, noiseSuppress = -21;

    speex_bits_init(&enc_bits);
    //enc_state = speex_encoder_init(&speex_wb_mode);
    enc_state = speex_encoder_init(speex_lib_get_mode(SPEEX_MODEID_WB));

    int speex_quality = 8;                                                  // speex 编码质量, 原来的值为 6, 8, 10
    speex_encoder_ctl(enc_state, SPEEX_SET_QUALITY, &speex_quality);        // 计划 10
    int speex_complexity = 6;                                               // 原来为 2
    speex_encoder_ctl(enc_state, SPEEX_SET_COMPLEXITY, &speex_complexity);  // 计划 10

    speex_encoder_ctl(enc_state, SPEEX_GET_FRAME_SIZE, &enc_frame_size);
    speex_encoder_ctl(enc_state, SPEEX_GET_SAMPLING_RATE, &sample_rate);
    //回声消除模块
    /* Echo canceller with 200 ms tail length */
    echo_state = speex_echo_state_init(frame_size, 10 * frame_size);
    speex_complexity = samplerate;
    speex_echo_ctl(echo_state, SPEEX_ECHO_SET_SAMPLING_RATE, &speex_complexity);

    /* Setup preprocessor and associate with echo canceller for residual echo suppression */
    preprocess = speex_preprocess_state_init(frame_size, samplerate);
    speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_ECHO_STATE, echo_state);
    speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_DENOISE, &denoise);               //降噪
    speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noiseSuppress);  //设置噪声的dB

    int speex_vbr = 0;
    speex_encoder_ctl(enc_state, SPEEX_SET_VBR, &speex_vbr);  // 新增 计划 0
    int speex_vad = 0;
    speex_encoder_ctl(enc_state, SPEEX_SET_VAD, &speex_vad);  // 1
    int speex_dtx = 0;
    speex_encoder_ctl(enc_state, SPEEX_SET_DTX, &speex_dtx);  // 新增 计划 0

    pkt_frame_count = 0;
#endif
    return true;
}

// note: 仅被析构使用
void SpeexEncoderContext::Reset() {
    std::lock_guard<std::mutex> lck(mtx_);
    if (enc_state) {
        speex_encoder_destroy(enc_state);
        enc_state = nullptr;
    }
    speex_bits_destroy(&enc_bits);
}

void SpeexEncoderContext::Encode(uint8_t *data, int size) {
    if (size > frame_size * 2) {
        return;
    }
    // 在 Windows 上注释 speex_preprocess_run 编码质量会有所改善
    // 但回音消除功能会有所削弱
    //    speex_preprocess_run(preprocess, (short *)data);
    speex_encode_int(enc_state, (short *)data, &enc_bits);

    pkt_frame_count++;
    if (pkt_frame_count == frames_per_packet) {
        memset(speex_buffer_, 0, frame_size * frames_per_packet);
        auto enc_size = speex_bits_write(&enc_bits, (char *)speex_buffer_, frame_size * frames_per_packet);
        speex_bits_reset(&enc_bits);
        // note: enc_size 返回了特别大的值
        assert(output_fn_);
        if (frame_size * frames_per_packet > enc_size) {
            output_fn_(audio_buffer_, 1+enc_size);
        }
        pkt_frame_count = 0;
    } else {
        output_fn_(audio_buffer_, 1);
    }
}

void AudioEncoder::Chunking(void *data, int size) {
    int postion = 0;
    while (postion < size) {
        auto copy_length = std::min(pcm.kChunk - pcm.postion, size - postion);
        memcpy(pcm.buffer + pcm.postion, static_cast<uint8_t*>(data) + postion, copy_length);
        postion += copy_length;
        pcm.postion += copy_length;

        if (pcm.postion == pcm.kChunk) {
            encoder_->Encode(pcm.buffer, pcm.postion);
            pcm.postion = 0;
        }
        assert(pcm.postion < pcm.kChunk);
    }
}

void AudioEncoder::Sampling(void *data, int size) {
    sampling.src.fillFrame((uint8_t *)data, size);
    sampling.convert();
    void *d = (void *)sampling.dst.Frame->data[0];
    int s = sampling.dst.Frame->nb_samples * sizeof(short);
    Chunking(d, s);
}
void AudioEncoder::Encode(void *data, size_t size) {
    if (audio.sampling()) {
        if (!sampling.DataInit()) {
            sampling.src.updateFrame(audio.src);
            sampling.dst.updateFrame(audio.dst);
            sampling.src.setCodecOptions(audio.src.nb_samples());  // 48k / 50
            sampling.dst.setCodecOptions(audio.dst.nb_samples());  // 16k / 50
            sampling.dst.fillFrame();
        }
        Sampling(data, size);
    } else {
        Chunking(data, size);
    }
}
AudioEncoder::AudioEncoder(bool use_opus_) {
    if (use_opus_) {
        encoder_ = std::make_shared<OpusEncoderContext>();
    } else {
        encoder_ = std::make_shared<SpeexEncoderContext>();
    }
    encoder_->Init();
}
}  // namespace audio
}  // namespace duobei
