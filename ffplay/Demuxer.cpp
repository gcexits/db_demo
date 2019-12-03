#include "Demuxer.h"

#include <string>

bool Demuxer::Open(void* param) {
    ifmt_ctx = (struct AVFormatContext*)param;
    need_free_ = false;
    int ret = avformat_find_stream_info(ifmt_ctx, NULL);
    if (ret < 0) {
        printf("Could not find stream information\n");
        return false;
    }
    videoindex = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    audioindex = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    mediaState.audio->stream = ifmt_ctx->streams[audioindex];
    video_decode.videoState.stream = ifmt_ctx->streams[videoindex];
    video_decode.OpenDecode(CodecPar(videoindex));
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

Demuxer::ReadStatus Demuxer::ReadFrame() {
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
        video_decode.Decode(pkt, 0);
        return ReadStatus::Video;
    } else if (pkt->stream_index == audioindex) {
        mediaState.audio->aacDecode.OpenDecode(CodecPar(audioindex));
        mediaState.audio->audioq.enQueue(pkt);
        return ReadStatus::Audio;
    } else {
        return ReadStatus::Subtitle;
    }
}