//
// Created by duobei on 2017/8/13.
//

#include "OpusEncoder.h"
#include <cassert>

namespace duobei {
namespace audio {

bool OpusEncoderContext::Init() {
    audio_buffer_[0] = 0xC6;
    opus_int32 sampling_rate = 16000;
    int channels = 1;
    opus_int32 bitrate_bps = 64000;
    int use_vbr = 1;
    int cvbr = 1;
    const char *strbandwidth = "FB";
    int complexity = 1;
    int err = 0;
    auto bandwidth = OPUS_BANDWIDTH_FULLBAND;

    int use_dtx;
    int packet_loss_perc;
    opus_int32 skip = 0;
    use_dtx = 0;
    packet_loss_perc = 100;

    if (sampling_rate != 8000 && sampling_rate != 12000 && sampling_rate != 16000 && sampling_rate != 24000 &&
        sampling_rate != 48000) {
        fprintf(stderr,
                "Supported sampling rates are 8000, 12000, "
                "16000, 24000 and 48000.\n");
        return false;
    }
    assert(frame_bytes == (sampling_rate * 2 * 20) / 1000);
    frame_size = frame_bytes / 2;
    // opusout = new char[frame_bytes];

    if (strbandwidth != nullptr) {
        if (strcmp(strbandwidth, "NB") == 0)
            bandwidth = OPUS_BANDWIDTH_NARROWBAND;
        else if (strcmp(strbandwidth, "MB") == 0)
            bandwidth = OPUS_BANDWIDTH_MEDIUMBAND;
        else if (strcmp(strbandwidth, "WB") == 0)
            bandwidth = OPUS_BANDWIDTH_WIDEBAND;
        else if (strcmp(strbandwidth, "SWB") == 0)
            bandwidth = OPUS_BANDWIDTH_SUPERWIDEBAND;
        else if (strcmp(strbandwidth, "FB") == 0)
            bandwidth = OPUS_BANDWIDTH_FULLBAND;
        else {
            fprintf(stderr,
                    "Unknown bandwidth %s. "
                    "Supported are NB, MB, WB, SWB, FB.\n",
                    strbandwidth);
            return false;
        }
    }

    enc = opus_encoder_create(sampling_rate, channels, /*OPUS_APPLICATION_VOIP*/ OPUS_APPLICATION_AUDIO, &err);
    if (err != OPUS_OK) {
        fprintf(stderr, "Cannot create encoder: %s\n", opus_strerror(err));
        return false;
    }
    /**
             *不可以设置其他参数，会消耗性能
             */
    opus_encoder_ctl(enc, OPUS_SET_BITRATE(bitrate_bps));
    //opus_encoder_ctl(enc,OPUS_SET_DTX(1));   //静音不传输
    //opus_encoder_ctl(enc, OPUS_SET_BITRATE(OPUS_AUTO));
    opus_encoder_ctl(enc, OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND));
    opus_encoder_ctl(enc, OPUS_SET_VBR(use_vbr));
    //
    opus_encoder_ctl(enc, OPUS_SET_VBR_CONSTRAINT(cvbr));
    opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY(complexity));
    opus_encoder_ctl(enc, OPUS_SET_FORCE_CHANNELS(channels));
    opus_encoder_ctl(enc, OPUS_SET_DTX(use_dtx));
    opus_encoder_ctl(enc, OPUS_SET_PACKET_LOSS_PERC(packet_loss_perc));
    opus_encoder_ctl(enc, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(enc, OPUS_SET_APPLICATION(OPUS_APPLICATION_RESTRICTED_LOWDELAY));

    opus_encoder_ctl(enc, OPUS_SET_INBAND_FEC(0));  //0:Disable, 1:Enable

    opus_encoder_ctl(enc, OPUS_GET_LOOKAHEAD(&skip));

    // note: 使用默认值
    // opus_encoder_ctl(enc, OPUS_SET_LSB_DEPTH(16));

    return true;
}

void OpusEncoderContext::Encode(uint8_t *in, int len, uint32_t ts) {
    int opus_len = 0;
    opus_int16 *frame = (opus_int16 *)in;

    int nbytes = opus_encode(enc, frame, frame_size, (uint8_t *)opusout, frame_bytes);
    if (nbytes > frame_bytes || nbytes < 0) {
        return;
    }
    opus_len = nbytes;
    assert(output_fn_);
    output_fn_((int8_t *)audio_buffer_, 1 + opus_len, ts, 1);
}

}  // namespace audio
}  // namespace duobei
