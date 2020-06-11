#include "mainPlayer.h"

static uint32_t sdl_refresh_timer_cb(uint32_t interval, void *opaque) {
    SDL_Event event;
    event.type = FF_REFRESH_EVENT;
    SDL_PushEvent(&event);
    return 0;
}

void schedule_refresh(int delay) {
    SDL_AddTimer(delay, sdl_refresh_timer_cb, nullptr);
}

int ffplay(Argument& cmd) {
    MediaState mediaState;
    assert(cmd.player.setMediaState(&mediaState));

    duobei::HttpFile httpFile;
    int ret = httpFile.Open(cmd.param.playerUrl);

    if (ret != duobei::FILEOK) {
        std::cout << "playerUrl is error" << std::endl;
        return -1;
    }
    // todo: 解封装 保存h264流有问题，需要av_bitstream_filter_init给h264裸流添加sps pps
    IOBufferContext ioBufferContext(0);
    Demuxer demuxer;

    std::thread read = std::thread([&] {
      int buffer_size = 1024 * 320;
      auto* buffer = new uint8_t[buffer_size + 1];
      while (1) {
          int hasRead_size = 0;
          int ret = httpFile.Read(buffer, buffer_size, buffer_size, hasRead_size);
          if (hasRead_size != 0) {
              ioBufferContext.FillBuffer(buffer, hasRead_size);
          }
          if (ret == duobei::FILEEND) {
              break;
          }
      }
    });

    std::thread readthread = std::thread([&] {
      bool status = false;
      void* param = nullptr;
      do {
          ioBufferContext.OpenInput();
          param = (AVFormatContext*)ioBufferContext.getFormatContext(&status);
      } while (!status);
      demuxer.Open(param, mediaState, cmd.player);
      while (1) {
          if (demuxer.ReadFrame(mediaState) == Demuxer::ReadStatus::EndOff) {
              break;
          }
      }
    });

    schedule_refresh(40);
    cmd.player.EventLoop(mediaState);
    ioBufferContext.io_sync.exit = true;
    if (readthread.joinable()) {
        readthread.join();
    }
    if (read.joinable()) {
        read.join();
    }
    httpFile.exit = true;
    httpFile.Close();
    std::cout << "curl file end" << std::endl;
    return 0;
}

#if defined(PARSE_SPS)
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef unsigned long DWORD;

UINT Ue(BYTE *pBuff, UINT nLen, UINT &nStartBit) {
    //计算0bit的个数
    UINT nZeroNum = 0;
    while (nStartBit < nLen * 8) {
        if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))  //&:按位与，%取余
        {
            break;
        }
        nZeroNum++;
        nStartBit++;
    }
    nStartBit++;

    //计算结果
    DWORD dwRet = 0;
    for (UINT i = 0; i < nZeroNum; i++) {
        dwRet <<= 1;
        if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) {
            dwRet += 1;
        }
        nStartBit++;
    }
    return (1 << nZeroNum) - 1 + dwRet;
}

int Se(BYTE *pBuff, UINT nLen, UINT &nStartBit) {
    int UeVal = Ue(pBuff, nLen, nStartBit);
    double k = UeVal;
    int nValue = ceil(k / 2);  //ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00
    if (UeVal % 2 == 0)
        nValue = -nValue;
    return nValue;
}

DWORD u(UINT BitCount, BYTE *buf, UINT &nStartBit) {
    DWORD dwRet = 0;
    for (UINT i = 0; i < BitCount; i++) {
        dwRet <<= 1;
        if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8))) {
            dwRet += 1;
        }
        nStartBit++;
    }
    return dwRet;
}

/**
 * H264的NAL起始码防竞争机制
 *
 * @param buf SPS数据内容
 *
 * @无返回值
 */
void de_emulation_prevention(BYTE *buf, unsigned int *buf_size) {
    int i = 0, j = 0;
    BYTE *tmp_ptr = NULL;
    unsigned int tmp_buf_size = 0;
    int val = 0;

    tmp_ptr = buf;
    tmp_buf_size = *buf_size;
    for (i = 0; i < (tmp_buf_size - 2); i++) {
        //check for 0x000003
        val = (tmp_ptr[i] ^ 0x00) + (tmp_ptr[i + 1] ^ 0x00) + (tmp_ptr[i + 2] ^ 0x03);
        if (val == 0) {
            //kick out 0x03
            for (j = i + 2; j < tmp_buf_size - 1; j++)
                tmp_ptr[j] = tmp_ptr[j + 1];

            //and so we should devrease bufsize
            (*buf_size)--;
        }
    }

    return;
}

/**
 * 解码SPS,获取视频图像宽、高信息
 *
 * @param buf SPS数据内容
 * @param nLen SPS数据的长度
 * @param width 图像宽度
 * @param height 图像高度

 * @成功则返回1 , 失败则返回0
 */
int h264_decode_sps(BYTE *buf, unsigned int nLen, int &width, int &height, int &fps) {
    UINT StartBit = 0;
    fps = 0;
    de_emulation_prevention(buf, &nLen);

    int forbidden_zero_bit = u(1, buf, StartBit);
    int nal_ref_idc = u(2, buf, StartBit);
    int nal_unit_type = u(5, buf, StartBit);
    if (nal_unit_type == 7) {
        int profile_idc = u(8, buf, StartBit);
        int constraint_set0_flag = u(1, buf, StartBit);  //(buf[1] & 0x80)>>7;
        int constraint_set1_flag = u(1, buf, StartBit);  //(buf[1] & 0x40)>>6;
        int constraint_set2_flag = u(1, buf, StartBit);  //(buf[1] & 0x20)>>5;
        int constraint_set3_flag = u(1, buf, StartBit);  //(buf[1] & 0x10)>>4;
        int reserved_zero_4bits = u(4, buf, StartBit);
        int level_idc = u(8, buf, StartBit);

        int seq_parameter_set_id = Ue(buf, nLen, StartBit);

        if (profile_idc == 100 || profile_idc == 110 ||
            profile_idc == 122 || profile_idc == 144) {
            int chroma_format_idc = Ue(buf, nLen, StartBit);
            if (chroma_format_idc == 3)
                int residual_colour_transform_flag = u(1, buf, StartBit);
            int bit_depth_luma_minus8 = Ue(buf, nLen, StartBit);
            int bit_depth_chroma_minus8 = Ue(buf, nLen, StartBit);
            int qpprime_y_zero_transform_bypass_flag = u(1, buf, StartBit);
            int seq_scaling_matrix_present_flag = u(1, buf, StartBit);

            int seq_scaling_list_present_flag[8];
            if (seq_scaling_matrix_present_flag) {
                for (int i = 0; i < 8; i++) {
                    seq_scaling_list_present_flag[i] = u(1, buf, StartBit);
                }
            }
        }
        int log2_max_frame_num_minus4 = Ue(buf, nLen, StartBit);
        int pic_order_cnt_type = Ue(buf, nLen, StartBit);
        if (pic_order_cnt_type == 0)
            int log2_max_pic_order_cnt_lsb_minus4 = Ue(buf, nLen, StartBit);
        else if (pic_order_cnt_type == 1) {
            int delta_pic_order_always_zero_flag = u(1, buf, StartBit);
            int offset_for_non_ref_pic = Se(buf, nLen, StartBit);
            int offset_for_top_to_bottom_field = Se(buf, nLen, StartBit);
            int num_ref_frames_in_pic_order_cnt_cycle = Ue(buf, nLen, StartBit);

            int *offset_for_ref_frame = new int[num_ref_frames_in_pic_order_cnt_cycle];
            for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
                offset_for_ref_frame[i] = Se(buf, nLen, StartBit);
            delete[] offset_for_ref_frame;
        }
        int num_ref_frames = Ue(buf, nLen, StartBit);
        int gaps_in_frame_num_value_allowed_flag = u(1, buf, StartBit);
        int pic_width_in_mbs_minus1 = Ue(buf, nLen, StartBit);
        int pic_height_in_map_units_minus1 = Ue(buf, nLen, StartBit);

        //width=(pic_width_in_mbs_minus1+1)*16;
        //height=(pic_height_in_map_units_minus1+1)*16;

        int frame_mbs_only_flag = u(1, buf, StartBit);
        if (!frame_mbs_only_flag)
            int mb_adaptive_frame_field_flag = u(1, buf, StartBit);

        int direct_8x8_inference_flag = u(1, buf, StartBit);
        int frame_cropping_flag = u(1, buf, StartBit);
        int frame_crop_left_offset = 0;
        int frame_crop_right_offset = 0;
        int frame_crop_top_offset = 0;
        int frame_crop_bottom_offset = 0;
        if (frame_cropping_flag) {
            frame_crop_left_offset = Ue(buf, nLen, StartBit);
            frame_crop_right_offset = Ue(buf, nLen, StartBit);
            frame_crop_top_offset = Ue(buf, nLen, StartBit);
            frame_crop_bottom_offset = Ue(buf, nLen, StartBit);
        }

        width = ((pic_width_in_mbs_minus1 + 1) * 16) - frame_crop_left_offset * 2 - frame_crop_right_offset * 2;
        height = ((2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 + 1) * 16) - (frame_crop_top_offset * 2) - (frame_crop_bottom_offset * 2);

        int vui_parameter_present_flag = u(1, buf, StartBit);
        if (vui_parameter_present_flag) {
            int aspect_ratio_info_present_flag = u(1, buf, StartBit);
            if (aspect_ratio_info_present_flag) {
                int aspect_ratio_idc = u(8, buf, StartBit);
                if (aspect_ratio_idc == 255) {
                    int sar_width = u(16, buf, StartBit);
                    int sar_height = u(16, buf, StartBit);
                }
            }
            int overscan_info_present_flag = u(1, buf, StartBit);
            if (overscan_info_present_flag)
                int overscan_appropriate_flagu = u(1, buf, StartBit);
            int video_signal_type_present_flag = u(1, buf, StartBit);
            if (video_signal_type_present_flag) {
                int video_format = u(3, buf, StartBit);
                int video_full_range_flag = u(1, buf, StartBit);
                int colour_description_present_flag = u(1, buf, StartBit);
                if (colour_description_present_flag) {
                    int colour_primaries = u(8, buf, StartBit);
                    int transfer_characteristics = u(8, buf, StartBit);
                    int matrix_coefficients = u(8, buf, StartBit);
                }
            }
            int chroma_loc_info_present_flag = u(1, buf, StartBit);
            if (chroma_loc_info_present_flag) {
                int chroma_sample_loc_type_top_field = Ue(buf, nLen, StartBit);
                int chroma_sample_loc_type_bottom_field = Ue(buf, nLen, StartBit);
            }
            int timing_info_present_flag = u(1, buf, StartBit);
            if (timing_info_present_flag) {
                int num_units_in_tick = u(32, buf, StartBit);
                int time_scale = u(32, buf, StartBit);
                fps = time_scale / (2 * num_units_in_tick);
            }
        }
        return true;
    } else
        return false;
}
#endif

int flv_player(Argument& cmd) {
    FlvPlayer flvPlayer;
    MediaState mediaState;
    assert(cmd.player.setMediaState(&mediaState));

    flvPlayer.startParse(cmd);
    cmd.player.EventLoop(mediaState);
    flvPlayer.stopParse();

    std::cout << "program exit" << std::endl;

    return 0;
}

bool playFrameData(uint8_t *data, int width, int height, int linesize) {
    static SDL_Window *screen = nullptr;
    static SDL_Renderer *sdlRenderer = nullptr;
    static SDL_Texture *sdlTexture = nullptr;
    static SDL_Rect sdlRect;
    static SDL_Event event;

    static char file[64] = {0};
    if (isprint(file[2]) == 0) {
        snprintf(file, 63, "window-%dx%d.yuv", width, height);
        {
            if (SDL_Init(SDL_INIT_VIDEO)) {
                exit(-1);
            }

            screen = SDL_CreateWindow("window",
                                      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                      width, height, SDL_WINDOW_OPENGL);

            if (!screen) {
                exit(-1);
            }
            sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
            sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);

            sdlRect.x = 0;
            sdlRect.y = 0;
            sdlRect.w = width;
            sdlRect.h = height;
        }
    }
    if (sdlTexture != nullptr) {
        SDL_UpdateTexture(sdlTexture, nullptr, data, linesize);
        SDL_RenderClear(sdlRenderer);
        SDL_RenderCopy(sdlRenderer, sdlTexture, nullptr, &sdlRect);
        SDL_RenderPresent(sdlRenderer);

        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                SDL_Quit();
                return false;
            }
        }
    }
    return true;
}

int ffmpeg_capture(Argument &cmd) {
    auto video_recorder_ = std::make_unique<duobei::capturer::VideoRecorder>(cmd.param.device.index, "Integrated Webcam", cmd.param.device.format.c_str());
    assert(video_recorder_->Open());

    AVPacket *dst_pkt = av_packet_alloc();
    bool running = true;
    while (running) {
        if (video_recorder_->Read()) {
            playFrameData(video_recorder_->frame_.data, video_recorder_->frame_.width, video_recorder_->frame_.height, video_recorder_->frame_.linesize);
            av_packet_unref(dst_pkt);
        }
    }
    video_recorder_->Close();

    return 0;
}

// todo: ffmpeg -re -stream_loop -1 -i wangyiyun.mp4 -vcodec copy -acodec copy -f flv rtmp://utx-live.duobeiyun.net/live/guochao
// todo: ffplay rtmp://htx-live.duobeiyun.net/live/guochao
int send_h264(Argument& cmd) {
    duobei::Time::Timestamp time;
    time.Start();
    int video_count = 0;
    duobei::RtmpObject rtmpObject(cmd.param.senderUrl, nullptr, 0);

    AVFormatContext* ifmt_ctx = nullptr;
    AVPacket pkt;
    int ret;
    int videoindex = -1, audioindex = -1;

    if ((ret = avformat_open_input(&ifmt_ctx, cmd.param.h264.c_str(), nullptr, nullptr)) < 0) {
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
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret == AVERROR_EOF) {
            assert(av_seek_frame(ifmt_ctx, -1, 0, AVSEEK_FLAG_BYTE) >= 0);
            video_count = 0;
            std::cout << "ok file" << std::endl;
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
            while (av_bsf_receive_packet(absCtx, &pkt) == 0);
            rtmpObject.sendH264Packet(pkt.data, pkt.size, keyFrame, time.Elapsed());
            video_count++;
        } else if (pkt.stream_index == audioindex) {
        }
        av_packet_unref(&pkt);
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }

    av_bsf_free(&absCtx);
    absCtx = nullptr;

    avformat_close_input(&ifmt_ctx);
    return 0;
}

int send_speex(Argument& cmd) {
    uint32_t audioTime = 0;
    duobei::audio::AudioEncoder audioEncoder(false);
    duobei::RtmpObject rtmpObject(cmd.param.senderUrl, nullptr, 0);
    audioEncoder.encoder_->output_fn_ = std::bind(&duobei::RtmpObject::sendAudioPacket, rtmpObject, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

    std::ifstream fp_in(cmd.param.pcm, std::ios::in);
    if (!fp_in.is_open()) {
        return -1;
    }
    char *buf_1 = new char[640];
    while (!fp_in.eof()) {
        fp_in.read(buf_1, 640);
        audioEncoder.Encode(buf_1, 640, audioTime);
        audioTime += 20;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    delete []buf_1;
    return 0;
}

int rtmp_recv(Argument &cmd) {
    duobei::RtmpObject rtmpObject(cmd.param.recvUrl, nullptr, 1);
    std::ofstream fp("/Users/guochao/Downloads/rtmp.flv", std::ios::out | std::ios::binary);

    while (1) {
        if (!RTMP_IsConnected(rtmpObject.rtmp)) {
            break;
        }
        RTMPPacket packet;
        while (RTMP_ReadPacket(rtmpObject.rtmp, &packet)) {

        }
    }
    return 0;
}
