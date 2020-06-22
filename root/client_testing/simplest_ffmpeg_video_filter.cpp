#include "simplest_ffmpeg_video_filter.h"
#include "mainPlayer.h"

#include "../../airPlayTest/duobei/parser/PacketParser.h"

int open_input_file(const char *filename) {
    int ret;
    AVCodec *dec;

    if ((ret = avformat_open_input(&fmt_ctx, filename, nullptr, nullptr)) < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(fmt_ctx, nullptr)) < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Cannot find a video stream in the input file\n");
        return ret;
    }
    video_stream_index = ret;

    dec_ctx = avcodec_alloc_context3(dec);
    if (!dec_ctx)
        return AVERROR(ENOMEM);
    avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[video_stream_index]->codecpar);

    if ((ret = avcodec_open2(dec_ctx, dec, nullptr)) < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Cannot open video decoder\n");
        return ret;
    }

    return 0;
}

int init_filters(const char *filters_descr) {
    char args[512];
    int ret = 0;
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVRational time_base = fmt_ctx->streams[video_stream_index]->time_base;
    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE};

    filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
             time_base.num, time_base.den,
             dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, nullptr, filter_graph);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Cannot create buffer source\n");
        goto end;
    }

    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       nullptr, nullptr, filter_graph);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Cannot create buffer sink\n");
        goto end;
    }

    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Cannot set output pixel format\n");
        goto end;
    }

    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
                                        &inputs, &outputs, nullptr)) < 0)
        goto end;

    if ((ret = avfilter_graph_config(filter_graph, nullptr)) < 0)
        goto end;

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}


int simplest_ffmpeg_video_filter(Argument &cmd) {
    duobei::parse_::PacketParser_ parser;
    duobei::video::H264Encoder h264Encoder;
    duobei::Time::Timestamp time;
    time.Start();
    int ret;
    AVPacket packet;
    AVFrame *frame;
    AVFrame *filt_frame;
#if defined(SAVEFILE)
    std::string out_yuv = "/Users/guochao/Downloads/1_告白气球_filter.h264";
    std::ofstream fp(out_yuv, std::ios::out | std::ios::binary | std::ios::app);
#endif
    uint8_t *data = new uint8_t[640 * 360 * 1.5];

    frame = av_frame_alloc();
    filt_frame = av_frame_alloc();
    if (!frame || !filt_frame) {
        perror("Could not allocate frame");
        exit(1);
    }

    if ((ret = open_input_file(cmd.param.filterUrl.c_str())) < 0)
        goto end;
    if ((ret = init_filters(filter_descr)) < 0)
        goto end;

    while (true) {
        ret = av_read_frame(fmt_ctx, &packet);
        if (ret == AVERROR_EOF) {
            std::cout << "read while" << std::endl;
            break;
        } else if (ret < 0) {
            std::cout << "read over" << std::endl;
            break;
        }

        if (packet.stream_index == video_stream_index) {
            ret = avcodec_send_packet(dec_ctx, &packet);
            if (ret < 0) {
                av_log(nullptr, AV_LOG_ERROR, "Error while sending a packet to the decoder\n");
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(dec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    av_log(nullptr, AV_LOG_ERROR, "Error while receiving a frame from the decoder\n");
                    goto end;
                }

                frame->pts = frame->best_effort_timestamp;

                if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                    av_log(nullptr, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
                    break;
                }

                while (1) {
                    ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    if (ret < 0)
                        goto end;
                    assert(filt_frame->format == AV_PIX_FMT_YUV420P);
                    auto buf_size = av_image_get_buffer_size((AVPixelFormat)filt_frame->format, filt_frame->width, filt_frame->height, 1);
                    assert(buf_size > 0);
                    ret = av_image_copy_to_buffer(data, buf_size, filt_frame->data, filt_frame->linesize, static_cast<AVPixelFormat >(filt_frame->format), filt_frame->width, filt_frame->height, 1);
                    assert(ret > 0);
                    time.Stop();
                    if (h264Encoder.DesktopEncode(data, filt_frame->width, filt_frame->height, 3)) {
#if defined(SAVEFILE)
                        fp.write(reinterpret_cast<char *>(h264Encoder.pkt->data), h264Encoder.pkt->size);
#else
                        parser.decodeH264Data(h264Encoder.pkt->data, h264Encoder.pkt->size, h264Encoder.pkt->flags & AV_PKT_FLAG_KEY ? 0 : 1, 0);
//                        rtmpObject.sendH264Packet(h264Encoder.pkt->data, h264Encoder.pkt->size, h264Encoder.pkt->flags & AV_PKT_FLAG_KEY, time.Elapsed());
#endif
                    }
                    av_frame_unref(filt_frame);
#if !defined(SAVEFILE)
                    std::this_thread::sleep_for(std::chrono::milliseconds(40));
#endif
                }
                av_frame_unref(frame);
            }
        }
        av_packet_unref(&packet);
    }
end:
    avfilter_graph_free(&filter_graph);
    avcodec_free_context(&dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_frame_free(&frame);
    av_frame_free(&filt_frame);

    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        exit(1);
    }
#if defined(SAVEFILE)
    fp.close();
#endif
    return 0;
}
