#include "RtmpSender.h"
#include <cassert>
#include <fstream>

namespace duobei {

bool RtmpObject::Init() {
    int ret = 0;
    rtmp = RTMP_Alloc();
    assert(rtmp);
    RTMP_Init(rtmp);
    ret = RTMP_SetupURL(rtmp, const_cast<char *>(url.c_str()));
    assert(ret == TRUE);
    RTMP_EnableWrite(rtmp);
    ret = RTMP_Connect(rtmp, nullptr);
    assert(ret == TRUE);
    ret = RTMP_ConnectStream(rtmp, 0);
    assert(ret);
#if 0
    {
        RTMPPack packet(1024*16, info->r->streamId());
        RTMPPacket_Alloc(&data, nSize);
        needFree_ = true;
        // 消息 0x03 音频 0x04 视频 0x06
        data.m_nChannel = 0x03; /* source channel (invoke) */
        data.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
        data.m_packetType = RTMP_PACKET_TYPE_INVOKE; /* INVOKE */
        data.m_nTimeStamp = 0;
        data.m_nInfoField2 = streamId;
        data.m_hasAbsTimestamp = 0;

        bodyEnc = data.m_body;
        bodyEnd = data.m_body + nSize;

        packet.data.m_headerType = RTMP_PACKET_SIZE_LARGE;

        packet.EncodeString(AMFConstant::connect);
        packet.EncodeNumber(1);
        packet.EncodeByte(AMF_OBJECT);

        auto app_ = net_node_->stream.app(m_);
        packet.EncodeNamedString(AMFConstant::app, StringToAVal(app_));
        packet.EncodeNamedNumber(AMFConstant::videoCodecs, 252);
        packet.EncodeNamedBoolean(AMFConstant::fpad, false);
        packet.EncodeNamedNumber(AMFConstant::audioCodecs, 3575);

        auto tcUrl = net_node_->stream.tcUrl(m_);
        packet.EncodeNamedString(AMFConstant::tcUrl, StringToAVal(tcUrl));
        packet.EncodeNamedNumber(AMFConstant::videoFunction, 1);
        packet.EncodeNamedNumber(AMFConstant::capabilities, 239);
        packet.EncodeNamedNumber(AMFConstant::objectEncoding, 3);

        packet.EncodeByte(0);
        packet.EncodeByte(0);
        packet.EncodeByte(AMF_OBJECT_END);
        packet.EncodeByte(AMF_OBJECT);

        packet.EncodeNamedNumber(AMFConstant::role, authInfo.userRole);
        packet.EncodeNamedString(AMFConstant::accessToken, StringToAVal(authInfo.accessToken));
        packet.EncodeNamedString(AMFConstant::uid, StringToAVal(authInfo.userId));
        packet.EncodeNamedString(AMFConstant::nickname, StringToAVal(authInfo.nickname));

        std::string clientType_ = authInfo.room1vn() ? "NEBULA_JZT" : "NEBULA_1V1";
        packet.EncodeNamedString(AMFConstant::clientType, StringToAVal(clientType_));

        packet.EncodeNamedNumber(AMFConstant::device, DEVICE_TYPE_IPAD);
        packet.EncodeByte(0);
        packet.EncodeByte(0);
        packet.EncodeByte(AMF_OBJECT_END);
        packet.EncodeEnd();

        return info->r->Connect(&packet.data);

    }
#endif
    ret = RTMP_IsConnected(rtmp);
    assert(ret == TRUE);
    return true;
}

size_t RtmpObject::packVideoSpsPps(char *body, const uint8_t *sps, size_t sps_len, const uint8_t *pps, size_t pps_len) {
    // AVC head
    size_t i = 0;
    body[i++] = 0x17;  //0
    body[i++] = 0x00;  //1
    body[i++] = 0x00;  //2
    body[i++] = 0x00;  //3
    body[i++] = 0x00;  //4

    // AVCDecoderConfigurationRecord
    body[i++] = 0x01;        //5
    body[i++] = sps[1];      //6
    body[i++] = sps[2];      //7
    body[i++] = sps[3];      //8
    body[i++] = (char)0xff;  //9

    // sps
    body[i++] = (char)0xe1;                   //10
    body[i++] = (char)(sps_len >> 8) & 0xff;  //11
    body[i++] = (char)(sps_len)&0xff;         //12
    memcpy(&body[i], sps, sps_len);
    i += sps_len;

    // pps
    body[i++] = 0x01;
    body[i++] = (char)(pps_len >> 8) & 0xff;
    body[i++] = (char)(pps_len)&0xff;
    memcpy(&body[i], pps, pps_len);
    i += pps_len;

    return i;
}

bool RtmpObject::send_video_sps_pps(const uint8_t *sps, int sps_len, const uint8_t *pps, int pps_len, uint32_t timestamp) {
    RTMPPacket data;
    RTMPPacket_Alloc(&data, 1024 + pps_len + sps_len);
    data.m_nChannel = 0x06;
    data.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    data.m_packetType = RTMP_PACKET_TYPE_VIDEO;
    data.m_nTimeStamp = timestamp;
    data.m_nInfoField2 = rtmp->m_stream_id;
    data.m_hasAbsTimestamp = 0;

    char *body = data.m_body;

    size_t i = packVideoSpsPps(body, sps, sps_len, pps, pps_len);
    data.m_nBodySize = (uint32_t)i;
    return RTMP_SendPacket(rtmp, &data, TRUE) == TRUE;
}

bool RtmpObject::send_video_only(const uint8_t *buf, int len, bool bKeyFrame, uint32_t timestamp) {
    RTMPPacket data;
    RTMPPacket_Alloc(&data, len + 9);
    data.m_nChannel = 0x06;
    data.m_headerType = RTMP_PACKET_SIZE_LARGE;
    data.m_packetType = RTMP_PACKET_TYPE_VIDEO;
    data.m_nTimeStamp = timestamp;
    data.m_nInfoField2 = rtmp->m_stream_id;
    data.m_hasAbsTimestamp = 0;
    data.m_nBodySize = len + 9;

    auto body = (uint8_t *)data.m_body;
    body[0] = bKeyFrame ? 0x17 : 0x27;

    body[1] = 0x01;
    body[2] = 0x00;
    body[3] = 0x00;
    body[4] = 0x00;

    body[5] = (len >> 24) & 0xff;
    body[6] = (len >> 16) & 0xff;
    body[7] = (len >> 8) & 0xff;
    body[8] = len & 0xff;
    memcpy(&body[9], buf, len);

    return RTMP_SendPacket(rtmp, &data, TRUE) == TRUE;
}

bool RtmpObject::sendH264Packet(uint8_t *buffer, int length, bool keyFrame, uint32_t timestamp, bool isdemux) {
    int index = 0;
    if (isdemux) {
//    if (buffer[4] == 0x06) {
        // todo: 循环跳过解封装的头部SEI帧
        //        if (parser_.isSEI(buffer, length)) {
        //
        //        }
        index = parser_.dumpHeader(buffer, length);
        buffer += index;
        length -= index;
        std::cout << "header is " << index << std::endl;
    }
    if (parser_.SPSPPS(buffer, length)) {
        auto status = send_video_sps_pps(buffer + parser_.spsBegin, parser_.spsLength, buffer + parser_.ppsBegin, parser_.ppsLength, timestamp);
        if (!status) {
            return false;
        }
        std::cout << "send_video_sps_pps" << std::endl;
    }
    return send_video_only(buffer + parser_.headerLength, length - parser_.headerLength, keyFrame, timestamp);
}

void RtmpObject::sendAudioPacket(const int8_t *buf, int len, uint32_t timestamp) {
    RTMPPacket data;
    RTMPPacket_Alloc(&data, len);

    data.m_nChannel = 0x04;
    data.m_headerType = RTMP_PACKET_SIZE_LARGE;
    data.m_packetType = RTMP_PACKET_TYPE_AUDIO;
    data.m_nTimeStamp = timestamp;
    data.m_nInfoField2 = rtmp->m_stream_id;
    data.m_hasAbsTimestamp = 0;

    data.m_nBodySize = len;
    memcpy(data.m_body, buf, len);

    if (RTMP_SendPacket(rtmp, &data, TRUE)) {
        std::cout << "send Audio Packet success" << std::endl;
    }
    return;
}
}