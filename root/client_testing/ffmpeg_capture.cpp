#include "ffmpeg_capture.h"

int ffmpeg_capture() {
    duobei::video::H264Encoder h264Encoder;
    duobei::RtmpObject rtmpObject("rtmp://utx-live.duobeiyun.net/live/guochao");

    auto video_recorder_ = std::make_unique<duobei::capturer::VideoRecorder>(1, "Integrated Webcam", "avfoundation");
    video_recorder_->Open();

    duobei::Time::Timestamp timestamp;
    AVPacket *dst_pkt = av_packet_alloc();
    while (1) {
        if (video_recorder_->Read()) {
            timestamp.Start();
            auto& frame_ = video_recorder_->frame_;
            if (!h264Encoder.DesktopEncode(const_cast<uint8_t*>(frame_.data), frame_.width, frame_.height, 3)) {
                continue;
            }
            av_packet_move_ref(dst_pkt, h264Encoder.pkt);
            timestamp.Stop();
            rtmpObject.sendH264Packet(dst_pkt->data, dst_pkt->size, dst_pkt->flags & AV_PKT_FLAG_KEY, timestamp.Elapsed(), false);
//            auto s = 66 - timestamp.Elapsed();
//            if (s > 0) {
//                std::this_thread::sleep_for(std::chrono::milliseconds(s));
//            }
            av_packet_unref(dst_pkt);
        }
    }
    video_recorder_->Close();

    return 0;
}