#pragma once

#include <cassert>
#include <functional>
#include <string>
#include <memory>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

namespace duobei::capturer {


class VideoRecorder {
    static std::string FixDeviceName(const std::string& deviceName) {
        std::string name{"video="};
        if (0 == deviceName.compare(0, name.length(), name)) {
            return deviceName;
        }
        return name + deviceName;
    }
    struct VideoSize {
        int width = 0;
        int height = 0;
    };
    VideoSize video_size;
public:
    explicit VideoRecorder(int di, const std::string &dn, const std::string &fn);
    VideoRecorder(const VideoRecorder &) = delete;

    ~VideoRecorder();
    bool Open();
    bool Read();

    static int AVIOInterruptCallback(void *opaque) {
        if (opaque == nullptr) return 1;
        auto that = reinterpret_cast<VideoRecorder *>(opaque);
        return that->InterruptCallback();
    }

    struct MyFrame {
        uint8_t *data = nullptr;
        std::unique_ptr<uint8_t[]> data_holder;
        int size = 0;
        int width = 0;
        int height = 0;
        int linesize = 0;
        void Update(uint8_t *data_, int size_, int width_, int height_, int linesize_) {
            size = size_;
            width = width_;
            height = height_;
            linesize = linesize_;

            if (!data_holder) {
                data_holder = std::make_unique<uint8_t []>(size);
                data = data_holder.get();
            }
            memcpy(data_holder.get(), data_, size);
        }
    } frame_;

private:
    int InterruptCallback() const {
        return 1;
    }

    int SetContextMac();

private:
    struct DeviceOption {
        std::string index;  // av_dict_set(&dict, "video_device_index", "1", 0);
        std::string name;   // "Microphone (High Definition Audio Device)" // special option for Windows
        std::string format;   // dshow
    };
    DeviceOption device_;

    struct OutputOption {
        int channel_layout = AV_CH_LAYOUT_MONO;
        int sample_rate = 16000;
        AVSampleFormat sample_fmt = AV_SAMPLE_FMT_S16;
        int nb_channels = 1;
    } out_;

    uint8_t **dst_data = nullptr;
    int dst_linesize = 0;
    int64_t src_nb_samples = 0;
    int64_t dst_nb_samples = 0;
    int64_t max_dst_nb_samples = 0;

public:
    // 设置输出参数
    void setOutputOption(int channel, int rate, int channels, AVSampleFormat sample_fmt) {
        out_.channel_layout = channel;
        out_.sample_rate = rate;
        out_.nb_channels = channels;
        out_.sample_fmt = sample_fmt;
        changed = true;
    }
    void Close() {
        needExit_ = true;
    }

private:
    int stream_index = -1;

    AVFormatContext *fmt_ctx = nullptr;
    AVCodecContext *codec_context_ = nullptr;

    AVPacket *packet = nullptr;
    AVFrame *frame = nullptr;
    AVFrame *scale_frame = nullptr;
    bool Scaling(int pix_fmt);
    void Play(AVFrame *frame);

    bool needExit_ = false;
    bool changed = true;
    SwrContext *swr_context = nullptr;
    SwsContext *sws_ctx = nullptr;

    bool Changed() {
        bool status = false;
        if (changed) {
            status = true;
            changed = false;
        }
        return status;
    }
};

}  // namespace duobei::capturer
