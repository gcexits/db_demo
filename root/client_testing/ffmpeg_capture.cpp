#include "ffmpeg_capture.h"

bool playFrameData(const uint8_t *data, int width, int height, int linesize) {
    static SDL_Window *screen = nullptr;
    static SDL_Renderer *sdlRenderer = nullptr;
    static SDL_Texture *sdlTexture = nullptr;
    static SDL_Rect sdlRect;

    static char file[64] = {0};
    if (isprint(file[2]) == 0) {
        snprintf(file, 63, "camera-%dx%d.yuv", width, height);
        {
            SDL_Init(SDL_INIT_VIDEO);
            screen = SDL_CreateWindow("ffmpeg-capture", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL);
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
    }
    return true;
}

int ffmpeg_capture() {
    auto video_recorder_ = std::make_unique<duobei::capturer::VideoRecorder>(0, "Integrated Webcam", "avfoundation");
    video_recorder_->Open();

    AVPacket *dst_pkt = av_packet_alloc();
    while (1) {
        if (video_recorder_->Read()) {
            playFrameData(video_recorder_->frame_.data, video_recorder_->frame_.width, video_recorder_->frame_.height, video_recorder_->frame_.linesize);
            av_packet_unref(dst_pkt);
        }
    }
    video_recorder_->Close();

    return 0;
}