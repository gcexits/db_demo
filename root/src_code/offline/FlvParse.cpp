#include "FlvParse.h"

void FlvPlayer::updateThread(Argument& cmd) {
    int index = 0;
    H264Decode video_decode;
    video_decode.OpenDecode(nullptr);
    SpeexDecode audio_decode;
    int width = 0;
    int height = 0;
    int videoIndex = 0;
    int audioIndex = 0;

    fp_in.open(cmd.param.flvUrl, std::ios::in);
    Header header;
    std::unique_lock<std::mutex> lock(readMtx_);
    fp_in.read((char*)header.header_data, 9);
    lock.unlock();
    Body buffer;

    while (!fp_in.eof() && !stop) {
        while (!hasPause) {
            std::unique_lock<std::mutex> lock(readMtx_);
            if (fp_in.eof()) {
                frame_.reset();
                fp_in.seekg(0, fp_in.beg);
            }
            if (hasPause || stop) {
                break;
            }
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
            currentTime = frame_.timestamp;

            fp_in.read((char*)frame_.body, frame_.body_length);

            if (frame_.tagType == 8) {
                audio_decode.Decode((char*)frame_.body + 1, frame_.body_length - 1);
                cmd.player.playAudio(1, 16000, 320);
                audioIndex++;
            } else if (frame_.tagType == 9) {
                int ret = getH264Data(frame_);
                if (!frame_.data) {
                    continue;
                }

                if (ret == kPpsSps) {
                    find_1st_key_frame_ = true;
                } else if (ret == kKeyVideo) {
                    video_decode.Decode(frame_.data, frame_.data_length);
                    frame_.data_length = 0;
                } else if (ret == kFullVideo) {
                    if (find_1st_key_frame_) {
                        video_decode.Decode(frame_.data, frame_.data_length);
                        frame_.data_length = 0;
                    }
                }
                videoIndex++;
            } else if (frame_.tagType == 18) {
                std::cout << "flv元数据 : " << frame_.body_length << " index = " << index << std::endl;
                AMFObject obj;
                int ret = AMF_Decode(&obj, (char*)frame_.body, frame_.body_length, FALSE);
                assert(ret >= 0);
                if (ret < 0) {
                    std::cout << "AMF_Decode failed" << std::endl;
                    delete[] frame_.body;
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
        while (hasPause) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    std::cout << "flv读取结束" << std::endl;

    fp_in.close();
}

void FlvPlayer::stopParse() {
    play();
    stop = true;
    if (parse.joinable()) {
        parse.join();
    }
}

int FlvPlayer::getH264Data(Frame& one_frame) {
    uint8_t* dataTmp = one_frame.body;

    Span sps;
    Span pps;
    if (!dataTmp[4] & 0x40) {
        std::cout << "Audio file" << std::endl;
    } else {
        std::cout << "Video file" << std::endl;
    }

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
            uint8_t* data = new uint8_t[sps.size];
            memcpy(data, sps.data, sps.size);
            h264_decode_sps(data, (unsigned int)sps.size, width, height, fps);
            delete[] data;
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

void FlvPlayer::seekTo(int64_t time) {
    uint8_t tagType = 0;
    uint32_t dataLen = 0;
    uint32_t timestamp = 0;
    uint8_t buf_h[15] = {0};

    play();
    std::unique_lock<std::mutex> lock(readMtx_);
    frame_.reset();
    time += currentTime;
    int seekTmp = 9;
    positionManager.seek(time, seekTmp);
    fp_in.seekg(0, fp_in.beg);
    fp_in.seekg(seekTmp - 4, fp_in.beg);

    while (true) {
        fp_in.read((char*)buf_h, 15);
        tagType = LEN1_((&buf_h[4]));
        dataLen = LEN3_((&buf_h[5]));
        timestamp = TIME4_((&buf_h[8]));
        if (timestamp == positionManager.previouskeyframetimest) {
            find_1st_key_frame_ = true;
            fp_in.seekg(-15, fp_in.cur);
            break;
        }
        fp_in.seekg(dataLen, fp_in.cur);
    }
    std::cout << "seek success over" << std::endl;
}

void FlvPlayer::pause() {
    std::lock_guard<std::mutex> lck(pauseMtx_);
    std::unique_lock<std::mutex> lock(readMtx_);
    hasPause = true;
}

void FlvPlayer::play() {
    std::lock_guard<std::mutex> lck(pauseMtx_);
    hasPause = false;
}