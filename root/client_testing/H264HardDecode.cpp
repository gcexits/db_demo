#include "H264HardDecode.h"

int decode_write(AVCodecContext *avctx, AVPacket *packet, HardwareContext &hardwareContext, std::ofstream &fp) {
    AVFrame *frame = nullptr, *sw_frame = nullptr;
    uint8_t *buffer = nullptr;
    int size;
    int ret = 0;
    YuvScaler yuvScaler;
    int rotWidth = 0;
    int rotHeight = 0;

    ret = avcodec_send_packet(avctx, packet);
    if (ret < 0) {
        fprintf(stderr, "Error during decoding\n");
        return ret;
    }

    while (1) {
        if (!(frame = av_frame_alloc()) || !(sw_frame = av_frame_alloc())) {
            fprintf(stderr, "Can not alloc frame\n");
            ret = AVERROR(ENOMEM);
            return -1;
        }

        ret = avcodec_receive_frame(avctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_frame_free(&frame);
            av_frame_free(&sw_frame);
            return 0;
        } else if (ret < 0) {
            fprintf(stderr, "Error while decoding\n");
            return -1;
        }

        frame->width = avctx->width;
        frame->height = avctx->height;

        hardwareContext.transfer(sw_frame, frame);

        size = av_image_get_buffer_size((enum AVPixelFormat)sw_frame->format, sw_frame->width,
                                        sw_frame->height, 1);
        buffer = new uint8_t[size];
        ret = av_image_copy_to_buffer(buffer, size,
                                      (const uint8_t *const *)sw_frame->data,
                                      (const int *)sw_frame->linesize, (enum AVPixelFormat)sw_frame->format,
                                      sw_frame->width, sw_frame->height, 1);
        if (ret < 0) {
            fprintf(stderr, "Can not copy image to buffer\n");
            delete[] buffer;
            return -1;
        }

        uint8_t *buf_yuv420p = new uint8_t[size];
        yuvScaler.YuvRote(buffer, sw_frame->height, sw_frame->width, 1, rotWidth, rotHeight, buf_yuv420p, 0, false);

        fp.write((char *)buf_yuv420p, size);

        av_frame_free(&frame);
        av_frame_free(&sw_frame);
        if (ret < 0)
            return ret;
    }
}

int h264HardDecode() {
    std::string hard_codec_name = "videotoolbox";
    std::string file_name = "/Users/guochao/Downloads/mda-hfapq015649s8777.mp4";
    std::string output_file = "./854x480.yuv";
    AVFormatContext *input_ctx = nullptr;
    int video_stream, ret;
    AVStream *video = nullptr;
    AVCodecContext *decoder_ctx = nullptr;
    AVCodec *decoder = nullptr;
    AVPacket packet;

    HardwareContext hardwareContext;

    if (avformat_open_input(&input_ctx, file_name.c_str(), nullptr, nullptr) != 0) {
        fprintf(stderr, "Cannot open input file '%s'\n", file_name.c_str());
        return -1;
    }

    if (avformat_find_stream_info(input_ctx, nullptr) < 0) {
        fprintf(stderr, "Cannot find input stream information.\n");
        return -1;
    }

    ret = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (ret < 0) {
        fprintf(stderr, "Cannot find a video stream in the input file\n");
        return -1;
    }
    video_stream = ret;

    if (!(decoder_ctx = avcodec_alloc_context3(decoder)))
        return AVERROR(ENOMEM);

    hardwareContext.find(hard_codec_name.c_str(), decoder);
    hardwareContext.init(decoder_ctx);

    video = input_ctx->streams[video_stream];
    if (avcodec_parameters_to_context(decoder_ctx, video->codecpar) < 0)
        return -1;

    if ((ret = avcodec_open2(decoder_ctx, decoder, nullptr)) < 0) {
        fprintf(stderr, "Failed to open codec for stream #%u\n", video_stream);
        return -1;
    }

    std::ofstream fp_(output_file, std::ofstream::out | std::ofstream::binary | std::ofstream::ate);

    while (ret >= 0) {
        if ((ret = av_read_frame(input_ctx, &packet)) < 0)
            break;

        if (video_stream == packet.stream_index)
            ret = decode_write(decoder_ctx, &packet, hardwareContext, fp_);

        av_packet_unref(&packet);
    }

    packet.data = nullptr;
    packet.size = 0;
    ret = decode_write(decoder_ctx, &packet, hardwareContext, fp_);
    av_packet_unref(&packet);

    if (fp_)
        fp_.close();
    avcodec_free_context(&decoder_ctx);
    avformat_close_input(&input_ctx);

    return 0;
}
