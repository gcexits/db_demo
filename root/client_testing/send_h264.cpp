#include "send_h264.h"

// todo: ffmpeg -re -stream_loop -1 -i wangyiyun.mp4 -vcodec copy -acodec copy -f flv rtmp://utx-live.duobeiyun.net/live/guochao
// todo: ffplay rtmp://htx-live.duobeiyun.net/live/guochao
int send_h264(Argument& cmd) {
    duobei::Time::Timestamp time;
    time.Start();
    int video_count = 0;
    duobei::RtmpObject rtmpObject("rtmp://utx-live.duobeiyun.net/live/guochao");

    AVFormatContext* ifmt_ctx = NULL;
    AVPacket pkt;
    int ret, i;
    int videoindex = -1, audioindex = -1;

    if ((ret = avformat_open_input(&ifmt_ctx, cmd.param.url.c_str(), nullptr, nullptr)) < 0) {
        printf("Could not open input file.");
        return -1;
    }
    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        printf("Failed to retrieve input stream information");
        return -1;
    }

    videoindex = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    audioindex = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    const AVBitStreamFilter* avBitStreamFilter = nullptr;
    AVBSFContext* absCtx = nullptr;

    avBitStreamFilter = av_bsf_get_by_name("h264_mp4toannexb");

    av_bsf_alloc(avBitStreamFilter, &absCtx);

    avcodec_parameters_copy(absCtx->par_in, ifmt_ctx->streams[videoindex]->codecpar);

    av_bsf_init(absCtx);

    int loop_count = 0;
    while (true) {
        int ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret == AVERROR_EOF) {
            assert(av_seek_frame(ifmt_ctx, -1, 0, AVSEEK_FLAG_BYTE) >= 0);
            video_count = 0;
            if (++loop_count > 10000) {
                break;
            }
            continue;
        }
        time.Stop();
        if (pkt.stream_index == videoindex) {
            bool keyFrame = pkt.flags & AV_PKT_FLAG_KEY;
            if (av_bsf_send_packet(absCtx, &pkt) != 0) {
                continue;
            }
            while (av_bsf_receive_packet(absCtx, &pkt) == 0) {
                continue;
            }
            rtmpObject.sendH264Packet(pkt.data, pkt.size, keyFrame, time.Elapsed(), video_count == 0);
            video_count++;
        } else if (pkt.stream_index == audioindex) {
        }
        av_packet_unref(&pkt);
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }

    av_bsf_free(&absCtx);
    absCtx = nullptr;

    avformat_close_input(&ifmt_ctx);

    if (ret < 0 && ret != AVERROR_EOF) {
        printf("Error occurred.\n");
        return -1;
    }
    return 0;
}

