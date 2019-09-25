#include <iostream>
#include <fstream>

#include "VideoRecorder.h"
#include "../h264_parse/H264Encoder.h"
#include "../utils/Time.h"

int main(int argc, char *argv[]) {
    duobei::video::H264Encoder h264Encoder;

    auto video_recorder_ = std::make_unique<duobei::capturer::VideoRecorder>(1, "Integrated Webcam", "avfoundation");
    video_recorder_->Open();

    duobei::Time::Timestamp timestamp;
    while (1) {
        if (video_recorder_->Read()) {
            timestamp.Start();
            auto& frame_ = video_recorder_->frame_;
            h264Encoder.DesktopEncode(const_cast<uint8_t*>(frame_.data), frame_.width, frame_.height, 3, 0);
            timestamp.Stop();
            auto s = 66 - timestamp.Elapsed();
            if (s > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(s));
            }
        }
    }
    video_recorder_->Close();

    return 0;
}