#include "send_h264.h"

// todo: ffmpeg -re -stream_loop -1 -i wangyiyun.mp4 -vcodec copy -acodec copy -f flv rtmp://utx-live.duobeiyun.net/live/guochao
// todo: ffplay rtmp://htx-live.duobeiyun.net/live/guochao
int send_h264() {
    duobei::Time::Timestamp time;
    time.Start();
    int video_count = 0;
    duobei::RtmpObject rtmpObject("rtmp://utx-live.duobeiyun.net/live/guochao");

    AVFormatContext* ifmt_ctx = NULL;
    AVPacket pkt;
    int ret, i;
    int videoindex = -1, audioindex = -1;
    const char* in_filename = "/Users/guochao/Downloads/3_一生所爱.mp4";

    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        printf("Could not open input file.");
        return -1;
    }
    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        printf("Failed to retrieve input stream information");
        return -1;
    }

    videoindex = -1;
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
        } else if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioindex = i;
        }
    }

    AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");

    while (av_read_frame(ifmt_ctx, &pkt) >= 0) {
        time.Stop();
        if (pkt.stream_index == videoindex) {
            bool keyFrame = pkt.flags & AV_PKT_FLAG_KEY;
            av_bitstream_filter_filter(h264bsfc, ifmt_ctx->streams[videoindex]->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
            rtmpObject.sendH264Packet(pkt.data, pkt.size, keyFrame, time.Elapsed(), video_count == 0);
            video_count++;
        } else if (pkt.stream_index == audioindex) {
        }
        av_free_packet(&pkt);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    av_bitstream_filter_close(h264bsfc);

    avformat_close_input(&ifmt_ctx);

    if (ret < 0 && ret != AVERROR_EOF) {
        printf("Error occurred.\n");
        return -1;
    }
    return 0;
}

