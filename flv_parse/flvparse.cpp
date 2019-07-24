//todo: 仅支持audio：speex/video：h264的flv数据，16000 1 s16

#include <fstream>
#include <iostream>
#include <list>
#include <thread>

#include <librtmp/amf.h>

#include "H264Decoder.h"
#include "SpeexDecoder.h"
#include "SDLPlayer.h"

//#define LOG
//#define PARSE_SPS

#define LEN4_(dataTmp) ((dataTmp[0] & 0x000000FF) << 24 | (dataTmp[1] & 0x000000FF) << 16 | (dataTmp[2] & 0x000000FF) << 8 | (dataTmp[3] & 0x000000FF))
#define LEN3_(dataTmp) ((dataTmp[0] & 0x000000FF) << 16 | (dataTmp[1] & 0x000000FF) << 8 | (dataTmp[2] & 0x000000FF))
#define LEN2_(dataTmp) ((dataTmp[0] & 0x000000FF) << 8 | (dataTmp[1] & 0x000000FF))
#define LEN1_(dataTmp) ((dataTmp[0] & 0x000000FF))
#define TIME4_(dataTmp) ((dataTmp[3] & 0x000000FF) << 24 | (dataTmp[0] & 0x000000FF) << 16 | (dataTmp[1] & 0x000000FF) << 8 | (dataTmp[2] & 0x000000FF))


#if defined(PARSE_SPS)
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

typedef  unsigned int UINT;
typedef  unsigned char BYTE;
typedef  unsigned long DWORD;

UINT Ue(BYTE *pBuff, UINT nLen, UINT &nStartBit)
{
    //计算0bit的个数
    UINT nZeroNum = 0;
    while (nStartBit < nLen * 8)
    {
        if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) //&:按位与，%取余
        {
            break;
        }
        nZeroNum++;
        nStartBit++;
    }
    nStartBit ++;


    //计算结果
    DWORD dwRet = 0;
    for (UINT i=0; i<nZeroNum; i++)
    {
        dwRet <<= 1;
        if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
        {
            dwRet += 1;
        }
        nStartBit++;
    }
    return (1 << nZeroNum) - 1 + dwRet;
}


int Se(BYTE *pBuff, UINT nLen, UINT &nStartBit)
{
    int UeVal=Ue(pBuff,nLen,nStartBit);
    double k=UeVal;
    int nValue=ceil(k/2);//ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00
    if (UeVal % 2==0)
        nValue=-nValue;
    return nValue;
}


DWORD u(UINT BitCount,BYTE * buf,UINT &nStartBit)
{
    DWORD dwRet = 0;
    for (UINT i=0; i<BitCount; i++)
    {
        dwRet <<= 1;
        if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
        {
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
void de_emulation_prevention(BYTE* buf,unsigned int* buf_size)
{
    int i=0,j=0;
    BYTE* tmp_ptr=NULL;
    unsigned int tmp_buf_size=0;
    int val=0;

    tmp_ptr=buf;
    tmp_buf_size=*buf_size;
    for(i=0;i<(tmp_buf_size-2);i++)
    {
        //check for 0x000003
        val=(tmp_ptr[i]^0x00) +(tmp_ptr[i+1]^0x00)+(tmp_ptr[i+2]^0x03);
        if(val==0)
        {
            //kick out 0x03
            for(j=i+2;j<tmp_buf_size-1;j++)
                tmp_ptr[j]=tmp_ptr[j+1];

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
int h264_decode_sps(BYTE * buf,unsigned int nLen,int &width,int &height,int &fps)
{
    UINT StartBit=0;
    fps=0;
    de_emulation_prevention(buf,&nLen);

    int forbidden_zero_bit=u(1,buf,StartBit);
    int nal_ref_idc=u(2,buf,StartBit);
    int nal_unit_type=u(5,buf,StartBit);
    if(nal_unit_type==7)
    {
        int profile_idc=u(8,buf,StartBit);
        int constraint_set0_flag=u(1,buf,StartBit);//(buf[1] & 0x80)>>7;
        int constraint_set1_flag=u(1,buf,StartBit);//(buf[1] & 0x40)>>6;
        int constraint_set2_flag=u(1,buf,StartBit);//(buf[1] & 0x20)>>5;
        int constraint_set3_flag=u(1,buf,StartBit);//(buf[1] & 0x10)>>4;
        int reserved_zero_4bits=u(4,buf,StartBit);
        int level_idc=u(8,buf,StartBit);

        int seq_parameter_set_id=Ue(buf,nLen,StartBit);

        if( profile_idc == 100 || profile_idc == 110 ||
            profile_idc == 122 || profile_idc == 144 )
        {
            int chroma_format_idc=Ue(buf,nLen,StartBit);
            if( chroma_format_idc == 3 )
                int residual_colour_transform_flag=u(1,buf,StartBit);
            int bit_depth_luma_minus8=Ue(buf,nLen,StartBit);
            int bit_depth_chroma_minus8=Ue(buf,nLen,StartBit);
            int qpprime_y_zero_transform_bypass_flag=u(1,buf,StartBit);
            int seq_scaling_matrix_present_flag=u(1,buf,StartBit);

            int seq_scaling_list_present_flag[8];
            if( seq_scaling_matrix_present_flag )
            {
                for( int i = 0; i < 8; i++ ) {
                    seq_scaling_list_present_flag[i]=u(1,buf,StartBit);
                }
            }
        }
        int log2_max_frame_num_minus4=Ue(buf,nLen,StartBit);
        int pic_order_cnt_type=Ue(buf,nLen,StartBit);
        if( pic_order_cnt_type == 0 )
            int log2_max_pic_order_cnt_lsb_minus4=Ue(buf,nLen,StartBit);
        else if( pic_order_cnt_type == 1 )
        {
            int delta_pic_order_always_zero_flag=u(1,buf,StartBit);
            int offset_for_non_ref_pic=Se(buf,nLen,StartBit);
            int offset_for_top_to_bottom_field=Se(buf,nLen,StartBit);
            int num_ref_frames_in_pic_order_cnt_cycle=Ue(buf,nLen,StartBit);

            int *offset_for_ref_frame=new int[num_ref_frames_in_pic_order_cnt_cycle];
            for( int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
                offset_for_ref_frame[i]=Se(buf,nLen,StartBit);
            delete [] offset_for_ref_frame;
        }
        int num_ref_frames=Ue(buf,nLen,StartBit);
        int gaps_in_frame_num_value_allowed_flag=u(1,buf,StartBit);
        int pic_width_in_mbs_minus1=Ue(buf,nLen,StartBit);
        int pic_height_in_map_units_minus1=Ue(buf,nLen,StartBit);

        //width=(pic_width_in_mbs_minus1+1)*16;
        //height=(pic_height_in_map_units_minus1+1)*16;

        int frame_mbs_only_flag=u(1,buf,StartBit);
        if(!frame_mbs_only_flag)
            int mb_adaptive_frame_field_flag=u(1,buf,StartBit);

        int direct_8x8_inference_flag=u(1,buf,StartBit);
        int frame_cropping_flag=u(1,buf,StartBit);
        int frame_crop_left_offset=0;
        int frame_crop_right_offset=0;
        int frame_crop_top_offset=0;
        int frame_crop_bottom_offset=0;
        if(frame_cropping_flag)
        {
            frame_crop_left_offset=Ue(buf,nLen,StartBit);
            frame_crop_right_offset=Ue(buf,nLen,StartBit);
            frame_crop_top_offset=Ue(buf,nLen,StartBit);
            frame_crop_bottom_offset=Ue(buf,nLen,StartBit);
        }

        width = ((pic_width_in_mbs_minus1 +1)*16) - frame_crop_left_offset*2 - frame_crop_right_offset*2;
        height= ((2 - frame_mbs_only_flag)* (pic_height_in_map_units_minus1 +1) * 16) - (frame_crop_top_offset * 2) - (frame_crop_bottom_offset * 2);

        int vui_parameter_present_flag=u(1,buf,StartBit);
        if(vui_parameter_present_flag)
        {
            int aspect_ratio_info_present_flag=u(1,buf,StartBit);
            if(aspect_ratio_info_present_flag)
            {
                int aspect_ratio_idc=u(8,buf,StartBit);
                if(aspect_ratio_idc==255)
                {
                    int sar_width=u(16,buf,StartBit);
                    int sar_height=u(16,buf,StartBit);
                }
            }
            int overscan_info_present_flag=u(1,buf,StartBit);
            if(overscan_info_present_flag)
                int overscan_appropriate_flagu=u(1,buf,StartBit);
            int video_signal_type_present_flag=u(1,buf,StartBit);
            if(video_signal_type_present_flag)
            {
                int video_format=u(3,buf,StartBit);
                int video_full_range_flag=u(1,buf,StartBit);
                int colour_description_present_flag=u(1,buf,StartBit);
                if(colour_description_present_flag)
                {
                    int colour_primaries=u(8,buf,StartBit);
                    int transfer_characteristics=u(8,buf,StartBit);
                    int matrix_coefficients=u(8,buf,StartBit);
                }
            }
            int chroma_loc_info_present_flag=u(1,buf,StartBit);
            if(chroma_loc_info_present_flag)
            {
                int chroma_sample_loc_type_top_field=Ue(buf,nLen,StartBit);
                int chroma_sample_loc_type_bottom_field=Ue(buf,nLen,StartBit);
            }
            int timing_info_present_flag=u(1,buf,StartBit);
            if(timing_info_present_flag)
            {
                int num_units_in_tick=u(32,buf,StartBit);
                int time_scale=u(32,buf,StartBit);
                fps=time_scale/(2*num_units_in_tick);
            }
        }
        return true;
    }
    else
        return false;
}

#endif

class Header {
public:
    uint8_t* header_data = nullptr;
    Header() {
        header_data = new uint8_t[9];
    }

    ~Header() {
        delete[] header_data;
    }
};

class Body {
public:
    uint8_t* body_data = nullptr;
    Body() {
        body_data = new uint8_t[15];
    }

    ~Body() {
        delete[] body_data;
    }
};

enum {
    kDontCare = 9,
    kPpsSps = 10,
    kFullVideo = 11,
    kKeyVideo = 12
};

class FlvPlayer {
public:
    struct PositionTimestamp {
        uint32_t timestamp;  //毫秒
        size_t position;
    };
    struct PositionManager {
        //flv info
        std::string metadatacreator;
        bool hasKeyframes = false;
        bool hasVideo = false;
        bool hasAudio = false;
        bool hasMetadata = false;
        bool canSeekToEnd = false;
        double duration = 0;
        double datasize = 0;
        double videosize = 0;
        double framerate = 0;
        double videodatarate = 0;
        double videocodecid = 0;
        double width = 0;
        double height = 0;
        double audiosize = 0;
        double audiodatarate = 0;
        double audiocodecid = 0;
        double audiosamplerate = 0;
        double audiosamplesize = 0;
        bool stereo = false;
        double filesize = 0;
        double lasttimestamp = 0;
        double lastkeyframetimest = 0;
        double lastkeyframelocati = 0;

        std::list<PositionTimestamp> _keyframes;  //文件快进位置
        bool hasFilepositionTimes = false;
        void push(double p4, double p5) {
            auto t = static_cast<uint32_t>(p5);
            auto ps = static_cast<size_t>(p4);
            _keyframes.push_back({t, ps});
        }
        void update() {
            if (!_keyframes.empty()) {
                hasFilepositionTimes = true;
                // cout << "解析元数据成功" << endl;
            }
        }

        int seek(int64_t s, int _headLen) {
            int seekTmp = _headLen;
            for (const auto& v : _keyframes) {
                if (v.timestamp > s) {
                    break;
                }
                seekTmp = v.position;
            }
            return seekTmp;
        }

        bool getFilepositionTimes(AMFObject& obj) {
            std::string text;
            AMFObjectProperty* p0 = AMF_GetProp(&obj, NULL, 1);
            if (p0 && (p0->p_type == AMF_ECMA_ARRAY || p0->p_type == AMF_OBJECT)) {
                AMFObject* obj0 = &p0->p_vu.p_object;
                AMFObjectProperty* p1 = NULL;
                for (int i = 0; i < obj0->o_num; i++) {
                    p1 = AMF_GetProp(obj0, NULL, i);
                    text = std::string(p1->p_name.av_val, p1->p_name.av_len);
                    if (text == "metadatacreator") {
                        metadatacreator = std::string(p1->p_vu.p_aval.av_val, p1->p_vu.p_aval.av_len);
                    } else if (text == "hasKeyframes") {
                        hasKeyframes = p1->p_vu.p_number;
                    } else if (text == "hasVideo") {
                        hasVideo = p1->p_vu.p_number;
                    } else if (text == "hasAudio") {
                        hasAudio = p1->p_vu.p_number;
                    } else if (text == "hasMetadata") {
                        hasMetadata = p1->p_vu.p_number;
                    } else if (text == "canSeekToEnd") {
                        canSeekToEnd = p1->p_vu.p_number;
                    } else if (text == "duration") {
                        duration = p1->p_vu.p_number;
                    } else if (text == "datasize") {
                        datasize = p1->p_vu.p_number;
                    } else if (text == "videosize") {
                        videosize = p1->p_vu.p_number;
                    } else if (text == "framerate") {
                        framerate = p1->p_vu.p_number;
                    } else if (text == "videodatarate") {
                        videodatarate = p1->p_vu.p_number;
                    } else if (text == "videocodecid") {
                        videocodecid = p1->p_vu.p_number;
                    } else if (text == "width") {
                        width = p1->p_vu.p_number;
                    } else if (text == "height") {
                        height = p1->p_vu.p_number;
                    } else if (text == "audiosize") {
                        audiosize = p1->p_vu.p_number;
                    } else if (text == "audiodatarate") {
                        audiodatarate = p1->p_vu.p_number;
                    } else if (text == "audiocodecid") {
                        audiocodecid = p1->p_vu.p_number;
                    } else if (text == "audiosamplerate") {
                        audiosamplerate = p1->p_vu.p_number;
                    } else if (text == "audiosamplesize") {
                        audiosamplesize = p1->p_vu.p_number;
                    } else if (text == "stereo") {
                        stereo = p1->p_vu.p_number;
                    } else if (text == "filesize") {
                        filesize = p1->p_vu.p_number;
                    } else if (text == "lasttimestamp") {
                        lasttimestamp = p1->p_vu.p_number;
                    } else if (text == "lastkeyframetimest") {
                        lastkeyframetimest = p1->p_vu.p_number;
                    } else if (text == "lastkeyframelocati") {
                        lastkeyframelocati = p1->p_vu.p_number;
                    } else if (text == "keyframes") {
                        AMFObjectProperty* p2 = AMF_GetProp(&p1->p_vu.p_object, NULL, 0);  //
                        text = std::string(p2->p_name.av_val, p2->p_name.av_len);
                        AMFObjectProperty* p3 = AMF_GetProp(&p1->p_vu.p_object, NULL, 1);  //
                        if (p2 && (p2->p_type == AMF_STRICT_ARRAY || p2->p_type == AMF_OBJECT) && p3 && (p3->p_type == AMF_STRICT_ARRAY || p3->p_type == AMF_OBJECT) && p2->p_vu.p_object.o_num == p3->p_vu.p_object.o_num) {
                            for (int j = 0; j < p2->p_vu.p_object.o_num; j++) {
                                AMFObjectProperty* p4 = AMF_GetProp(&p2->p_vu.p_object, NULL, j);  //
                                AMFObjectProperty* p5 = AMF_GetProp(&p3->p_vu.p_object, NULL, j);  //
                                push(p4->p_vu.p_number, p5->p_vu.p_number * 1000.0);
                            }
                        }
                    }
                }
            }
            update();
            return true;
        }
    };
    PositionManager positionManager;

    bool find_1st_key_frame_ = false;  //是否是首次视频帧，保证首次视频是关键帧

    enum {
        kBufferSize = 256,
        kBodyBufferSize = 512 * 1024,
        kDataLength = 1024 * 1280
    };
    struct Span {
        const uint8_t* data;
        int size;
        const uint8_t* DataEnd() const {
            return data + size;
        }
    };

    struct Frame {
        uint8_t* data = nullptr;
        size_t data_length = 0;
        void WriteData(uint8_t* d, int l) {
            assert(data_length + l <= kDataLength);
            memcpy(data + data_length, d, l);
            data_length += l;
        }
        void WriteData(const Span& span) {
            assert(data_length + span.size <= kDataLength);
            memcpy(data + data_length, span.data, span.size);
            data_length += span.size;
        }
        void WriteH264Header() {
            uint8_t h264begin[] = {0x00, 0x00, 0x00, 0x01};
            WriteData(h264begin, 4);
        }

        Frame() {
            data = new uint8_t[kDataLength];
        }
        ~Frame() {
            if (data) {
                delete[] data;
            }
            reset();
        }

        uint8_t tagType = 0;       //帧类型
        int32_t timestamp = 0;     //时间戳
        uint32_t streamsId = 0;    //
        uint8_t* body = nullptr;   //帧数据
        uint32_t body_length = 0;  //帧数据长度

        const uint8_t* BodyEnd() const {
            return body + body_length;
        }

        void reset() {
            if (body != nullptr) {
                delete body;
                body = nullptr;
            }
        }
    };

    struct NaluHelper {
        // todo：NALU包长度字节数
        int naluPackageLenBits = -1;

        int CheckNalu(Frame& one_frame, uint8_t* dataTmp, int default_value) {
            // note：NALU
            int nowlen = 0;
            int nNauluLen = 0;
            // note：cts：0x000000，三个字节
            dataTmp += 5;
            while (nowlen < one_frame.body_length - 5) {
                switch (naluPackageLenBits) {
                    case 4:
                        nNauluLen = LEN4_((&(dataTmp[0])));
                        break;
                    case 3:
                        nNauluLen = LEN3_((&(dataTmp[0])));
                        break;
                    case 2:
                        nNauluLen = LEN2_((&(dataTmp[0])));
                        break;
                    case 1:
                        nNauluLen = LEN1_((&(dataTmp[0])));
                        break;
                    default:
                        return kDontCare;
                }
                dataTmp += naluPackageLenBits;
                nowlen += (naluPackageLenBits + nNauluLen);

                one_frame.WriteH264Header();

                if (dataTmp + nNauluLen > one_frame.BodyEnd()) {
                    return kDontCare;
                }

                one_frame.WriteData(dataTmp, nNauluLen);
                dataTmp += nNauluLen;
            }
            return default_value;
        }
    };
    NaluHelper nalu_helper;
public:
    Frame frame_;

    void getVideoAudioData() {
        int index = 0;

        H264Decode video_decode;
        SpeexDecode audio_decode;
        uint8_t* yuv = new uint8_t[1920 * 1080 * 3 / 2];
        uint8_t* pcm = new uint8_t[640];
        int width = 0;
        int height = 0;

        std::ifstream fp_in;
//        fp_in.open("/Users/guochao/DBY_code/ff_test/mv.flv", std::ios::in);
//        fp_in.open("/Users/guochao/Downloads/out-video-fc94ef156a6b40abb7aa7d1c4fde7df1_f_1491816916000_t_1491818181266.flv", std::ios::in);
        fp_in.open("/Users/guochao/DBY_code/ff_test/1.flv", std::ios::in);
//        fp_in.open("/Users/guochao/Downloads/out-video-jz7ba5f98319bb48f2bb26cabe7d956045_f_1542686509594_t_1542687353424.flv", std::ios::in);
//        fp_in.open("//Users/guochao/Downloads/out-video-acb0c0bd325342319dadf48e1772fc49_f_1561097680263_t_1561100555093.flv", std::ios::in);
//        fp_in.open("/Users/guochao/Downloads/out-audio-0aee7de51c5b44b8a5f345b146f83809-vs_f_1561097554498_t_1561097988738.flv", std::ios::in);
//        fp_in.open("/Users/guochao/Downloads/crop.flv", std::ios::in);
#if !defined(USING_SDL)
        std::ofstream fp_out_audio;
        fp_out_audio.open("./haha.pcm", std::ios::ate | std::ios::out | std::ios::binary);
        std::ofstream fp_out_video;
        fp_out_video.open("./haha.yuv", std::ios::ate | std::ios::out | std::ios::binary);
#endif
        Header header;
        fp_in.read((char*)header.header_data, 9);
        Body buffer;

        while (!fp_in.eof()) {
            fp_in.read((char*)buffer.body_data, 4);
            // todo: 返回上一次read的字节数
            if (fp_in.gcount() == 0) {
                break;
            }
            fp_in.read((char*)buffer.body_data, 11);
            if (fp_in.gcount() == 0) {
                break;
            }
            frame_.tagType = buffer.body_data[0];
            frame_.body_length = LEN3_((&buffer.body_data[1]));
            frame_.timestamp = TIME4_((&buffer.body_data[4]));
            frame_.body = new uint8_t[frame_.body_length];

            fp_in.read((char*)frame_.body, frame_.body_length);

            if (frame_.tagType == 8) {
#if defined(LOG)
                std::cout << "音频数据 : " << frame_.body_length << " index = " << index << std::endl;
#endif
                audio_decode.Decode((char *)frame_.body + 1, frame_.body_length - 1, pcm);
#if !defined(USING_SDL)
                    fp_out_audio.write((char *)pcm, 640);
#endif
            } else if (frame_.tagType == 9) {
#if defined(LOG)
                std::cout << "视频数据 : " << frame_.body_length << " index = " << index << std::endl;
#endif
                int ret = getH264Data(frame_);
                if (!frame_.data) {
                    continue;
                }

                if (ret == kPpsSps) {
#if defined(LOG)
                    std::cout << "pps sps : " << frame_.body_length << " index = " << index << std::endl;
#endif
                    find_1st_key_frame_ = true;
                } else if (ret == kKeyVideo) {
                    video_decode.Decode(frame_.data, frame_.data_length, width, height, yuv);
                    frame_.data_length = 0;
                } else if (ret == kFullVideo) {
                    if (find_1st_key_frame_) {
                        video_decode.Decode(frame_.data, frame_.data_length, width, height, yuv);
                        frame_.data_length = 0;
                    }
                }
                if (video_decode.success) {
#if !defined(USING_SDL)
                    fp_out_video.write((char *)yuv, width * height * 1.5);
#endif
                }
//            fp_in.seekg(flvPlayer.frame_.body_length, fp_in.cur);
            } else if (frame_.tagType == 18) {
                std::cout << "flv元数据 : " << frame_.body_length << " index = " << index << std::endl;
                AMFObject obj;
                int ret = AMF_Decode(&obj, (char*)frame_.body, frame_.body_length, FALSE);
                if (ret < 0) {
                    std::cout << "AMF_Decode failed" << std::endl;
                    delete [] frame_.body;
                    break;
                }
                positionManager.getFilepositionTimes(obj);
                AMF_Reset(&obj);
            } else {
                std::cout << "unknow : " << frame_.body_length << " type : " << frame_.tagType << std::endl;
                fp_in.seekg(frame_.body_length, fp_in.cur);
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
            index++;
            frame_.reset();
        }
        std::cout << "flv读取结束" << std::endl;
        delete[] pcm;
        delete[] yuv;

#if !defined(USING_SDL)
        fp_out_audio.close();
        fp_out_video.close();
#endif

    }
    void startParse() {
        parse = std::thread(&FlvPlayer::getVideoAudioData, this);
    }
    std::thread parse;

    void stopParse() {
        if (parse.joinable()) {
            parse.join();
        }
    }

    int getH264Data(Frame& one_frame) {
        uint8_t *dataTmp = one_frame.body;

        Span sps;
        Span pps;

        if (dataTmp[0] == 0x17) {
            // note：onMetadata：sps/pps
            if (dataTmp[1] == 0x00) {  //AVCDecoderConfigurationRecord
                // note：cts 0x000000，三个字节
                nalu_helper.naluPackageLenBits = (dataTmp[9] & 0x03) + 1;

                int numOfSequenceParameterSets = dataTmp[10] & 0x1F;  //sps数量
                dataTmp += 11;

                for (int i = 0; i < numOfSequenceParameterSets; i++) {  //(data[num]&0x000000FF)<<24|(data[num+1]&0x000000FF)<<16|(data[num+2]&0x000000FF)<<8|data[num+3]&0x000000FF;
                    // note：pps length
                    int sequenceParameterSetLength = LEN2_((&(dataTmp[0])));
                    sps.data = dataTmp + 2;
                    sps.size = sequenceParameterSetLength;

                    dataTmp += 2;
                    dataTmp += sequenceParameterSetLength;
                }
#if defined(PARSE_SPS)
                int height = 0;
                int width = 0;
                int fps = 0;
                uint8_t *data = new uint8_t[sps.size];
                memcpy(data, sps.data, sps.size);
                h264_decode_sps(data, (unsigned int)sps.size, width, height, fps);
                delete []data;
                std::cout << width << " " << height << " " << fps << std::endl;
#endif
                int numOfPictureParameterSets = LEN1_((&(dataTmp[0])));
                dataTmp += 1;
                for (int i = 0; i < numOfPictureParameterSets; i++) {
                    int pictureParameterSetLength = LEN2_((&(dataTmp[0])));
                    pps.data = dataTmp + 2;
                    pps.size = pictureParameterSetLength;
                    dataTmp += 2;
                    dataTmp += pictureParameterSetLength;
                }
                one_frame.data_length = 0;
                one_frame.WriteH264Header();

                if (sps.DataEnd() > one_frame.BodyEnd()) {
                    return kDontCare;
                }
                one_frame.WriteData(sps);
                one_frame.WriteH264Header();

                if (pps.DataEnd() > one_frame.BodyEnd()) {
                    return kDontCare;
                }
                one_frame.WriteData(pps);
                return kPpsSps;
            } else if (dataTmp[1] == 0x01) {
                return nalu_helper.CheckNalu(one_frame, dataTmp, kKeyVideo);
            }
        } else if (dataTmp[0] == 0x27) {
            if (!find_1st_key_frame_)
                return kFullVideo;
            if (dataTmp[1] == 0x01) {
                return nalu_helper.CheckNalu(one_frame, dataTmp, kFullVideo);
            }
        }
        return kDontCare;
    }
};

void RegisterPlayer() {
    using namespace std::placeholders;

    SDLPlayer *player = SDLPlayer::getPlayer();
    AVRegister::setinitVideoPlayer(std::bind(&SDLPlayer::openVideo, player, _1, _2));
    AVRegister::setinitPcmPlayer(std::bind(&SDLPlayer::openAudio, player, _1));
}

int main() {
    RegisterPlayer();
    FlvPlayer flvPlayer;

    flvPlayer.startParse();
    SDLPlayer::getPlayer()->EventLoop();
    flvPlayer.stopParse();

    return 0;
}