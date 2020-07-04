## 简介

airPlayTest还未合进整个项目

支持windows/Mac/ios/Android

## 编译

`Mac` 使用 `clion` 编译

`Windows` 使用 `clion` 配合 mingw32 编译, airPlayTest 使用 `vs2019`

`ios` 使用 `xcode`

`Android` 使用 `ndk-16`

## 依赖

- 采用 c++17 标准
- 依赖 curl, speex, rtmp, ffmpeg, opus, sdl2, libyuv, airplay

## 使用

根据Configuration_profile.json进行配置实现不同功能, 结合root/client_testing/main.cpp 使用

launch
- 1: curl + ffplay http拉流播放器
- 2: 桌面共享/屏幕采集
- 3: http-flv
- 4: c++ 特性简单测试代码
- 5: send-speex-rtmp 推流
- 6: send-h264-rtmp 推流
- 7: yuv处理, 旋转镜像转换等
- 8: 音频处理混音
- 6: rapidjson
- 6: recv-stream-rtmp 拉流