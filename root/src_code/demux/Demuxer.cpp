#include "Demuxer.h"

#include <string>

bool Demuxer::Open(void* param, struct MediaState& m, SDLPlayer& player) {
    ifmt_ctx = (struct AVFormatContext*)param;
    need_free_ = false;
    int ret = avformat_find_stream_info(ifmt_ctx, NULL);
    if (ret < 0) {
        printf("Could not find stream information\n");
        return false;
    }
    videoindex = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    audioindex = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    m.videoState.stream = ifmt_ctx->streams[videoindex];
    m.videoState.videoDecode.OpenDecode(CodecPar(videoindex));

    m.audioState.audioDecode.OpenDecode(CodecPar(audioindex));

    player.playAudio(m.audioState.audioDecode.codecCtx_->channels, m.audioState.audioDecode.codecCtx_->sample_rate, FFMAX(512, 2 << av_log2(m.audioState.audioDecode.codecCtx_->sample_rate / 30)));
    m.audioState.decode = std::thread(&AudioState_1::audio_thread, &m.audioState);
    m.videoState.decode = std::thread(&VideoState_1::video_thread, &m.videoState);
    opened_ = true;
    return true;
}

bool Demuxer::addSpsPps(AVPacket* pkt, AVCodecParameters* codecpar, std::string name) {
    const AVBitStreamFilter* avBitStreamFilter = nullptr;
    AVBSFContext* absCtx = NULL;

    avBitStreamFilter = av_bsf_get_by_name(name.c_str());

    av_bsf_alloc(avBitStreamFilter, &absCtx);

    avcodec_parameters_copy(absCtx->par_in, codecpar);

    av_bsf_init(absCtx);

    if (av_bsf_send_packet(absCtx, pkt) != 0) {
        return false;
    }

    while (av_bsf_receive_packet(absCtx, pkt) == 0) {
        continue;
    }

    av_bsf_free(&absCtx);
    absCtx = NULL;
    return true;
}

Demuxer::ReadStatus Demuxer::ReadFrame(struct MediaState& m) {
    if (exit) {
        return ReadStatus::EndOff;
    }
    int ret = av_read_frame(ifmt_ctx, pkt);
    if (ret < 0) {
        if (ret == AVERROR_EOF || avio_feof(ifmt_ctx->pb)) {
            return ReadStatus::EndOff;
        }
        //av_seek_frame(ifmt_ctx, pkt->stream_index, 0, AVSEEK_FLAG_BACKWARD);
        return ReadStatus::Error;
    }
    if (pkt->stream_index == videoindex) {
        // todo: 给h264裸流添加sps pps
        addSpsPps(pkt, CodecPar(videoindex), "h264_mp4toannexb");
        m.videoState.packetData.packet_queue_put(pkt);
        av_packet_unref(pkt);
        return ReadStatus::Video;
    } else if (pkt->stream_index == audioindex) {
        m.audioState.packetData.packet_queue_put(pkt);
        av_packet_unref(pkt);
        return ReadStatus::Audio;
    } else {
        return ReadStatus::Subtitle;
    }
}