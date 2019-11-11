#include "../utils/Time.h"
#include "H264Encoder.h"

#include <thread>
#include <fstream>

#define USE_H264BSF 1

int mainDemux() {
//    avformat_write_header();
    std::ofstream fp_out("./test_demux.h264", std::ios::binary | std::ios::ate);
    duobei::Time::Timestamp time;
    time.Start();
    int video_count = 0;
    duobei::RtmpObject rtmpObject("rtmp://utx-live.duobeiyun.net/live/guochao");

    AVFormatContext* ifmt_ctx = NULL;
    AVPacket pkt;
    int ret, i;
    int videoindex = -1, audioindex = -1;
    const char* in_filename = "/Users/guochao/Downloads/wangyiyun.mp4";
//    const char* in_filename = "/Users/guochao/Downloads/youtube.webm";
//    const char* in_filename = "/Users/guochao/Downloads/out-video-jze002ed10fa274d69994bdea9685e8069_f_1556449161787_t_1556450882947.flv";
//    const char* in_filename = "/Users/guochao/Downloads/douyin_3.mp4";

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

#if USE_H264BSF
    AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
#endif

    while (av_read_frame(ifmt_ctx, &pkt) >= 0) {
        if (pkt.stream_index == videoindex) {
            time.Stop();
            bool keyFrame = pkt.flags & AV_PKT_FLAG_KEY;
#if USE_H264BSF
            av_bitstream_filter_filter(h264bsfc, ifmt_ctx->streams[videoindex]->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#else
//            if (keyFrame) {
//                uint8_t h264header[] = {0x00, 0x00, 0x00, 0x01};
//                memcpy(pkt.data, h264header, 4);
//                uint8_t sps[] = {0x67, 0x4D, 0x40, 0x1F, 0xEC, 0xA0, 0x28, 0x02, 0xDD, 0x80, 0x88, 0x00, 0x00, 0x03, 0x00, 0x08,
//                                 0x00, 0x00, 0x03, 0x01, 0x90, 0x78, 0xC1, 0x8C, 0xB0};
//                memcpy(pkt.data + 4, sps, 25);
//                uint8_t pps[] = {0x68, 0xEB, 0xE3, 0xCB, 0x20};
//                memcpy(pkt.data + 4 + 25, h264header, 4);
//                memcpy(pkt.data + 4 + 25 + 4, pps, 5);
//                memcpy(pkt.data + 4 + 25 + 4 + 5, h264header, 4);
//                pkt.size += 38;
//                pkt.size += 4;
//            }
#endif
//            fp_out.write((char *)pkt.data, pkt.size);
            // note: header 688
//            if (video_count == 0) {
//                rtmpObject.sendH264Packet(pkt.data + 688, pkt.size - 688, keyFrame, time.Elapsed());
//            }
            rtmpObject.sendH264Packet(pkt.data, pkt.size, keyFrame, time.Elapsed(), video_count == 0);
            video_count++;
        } else if (pkt.stream_index == audioindex) {
        }
        av_free_packet(&pkt);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

#if USE_H264BSF
    av_bitstream_filter_close(h264bsfc);
#endif

    avformat_close_input(&ifmt_ctx);

    if (ret < 0 && ret != AVERROR_EOF) {
        printf("Error occurred.\n");
        return -1;
    }
    return 0;
}

int mainEncode() {
    duobei::video::H264Encoder h264Encoder;
    int video_count = 0;

    std::ifstream fp_in("/Users/guochao/Downloads/test_1280x720.yuv", std::ios::binary);
    int srcW = 1280, srcH = 720;
    int bufSize = srcW * srcH * 1.5;

    uint8_t *buf = new uint8_t[bufSize + 1];
    duobei::Time::Timestamp time;
    time.Start();

    while (!fp_in.eof()) {
        time.Stop();
        if (video_count == 1) {
            return -1;
        }
        fp_in.read((char *)buf, bufSize);
        h264Encoder.DesktopEncode(buf, srcW, srcH, 3, time.Elapsed());
        video_count++;
//        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }

    fp_in.close();

    return 0;
}

//ffmpeg -re -stream_loop -1 -i wangyiyun.mp4 -vcodec copy -acodec copy -f flv rtmp://utx-live.duobeiyun.net/live/guochao
//ffplay rtmp://htx-live.duobeiyun.net/live/guochao
int main(int argc, char* argv[]) {
    avdevice_register_all();
    auto fmt_ctx = avformat_alloc_context();
    AVDictionary* options = nullptr;
    av_dict_set(&options, "framerate", "30", 0);
    av_dict_set(&options, "video_device_index", "0", 0);
    av_dict_set(&options, "video_size", "1280x720", 0);
    av_dict_set(&options, "pixel_format", "nv12", 0);
//    av_dict_set(&options, "list_devices", "true", 0);
    auto fmt = av_find_input_format("avfoundation");
    std::cout << "begin" << std::endl;
    auto ret = avformat_open_input(&fmt_ctx, "", fmt, &options);
    std::cout << "end" << std::endl;
    std::cout << ret << " " << av_err2str(ret) << std::endl;
    return 0;
//    return mainDemux();
//    return mainEncode();
}
