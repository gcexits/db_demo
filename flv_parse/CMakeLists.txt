cmake_minimum_required(VERSION 3.7)
project(flv)

# 支持c++14
SET(CMAKE_C_COMPILER g++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

# 提高编译警告级别
add_definitions(-Wall)

set(EXE_TARGET_NAME flv)

set(ROOT ${CMAKE_CURRENT_LIST_DIR})
# message("${ROOT}")
set(USR /usr/local/Cellar)

set(SOURCE_FILES
        ${ROOT}/flvplayer.cpp
        ${ROOT}/H264Decoder.cpp
        ${ROOT}/SpeexDecoder.cpp
        ${ROOT}/Optional.cpp
        ${ROOT}/SDLPlayer.cpp
        ${ROOT}/HttpClient.cpp
        ${ROOT}/../yuv_deal/yuvscale.cpp
        ${ROOT}/../Time.cpp
        ${ROOT}/hlring/RingBuffer.cpp
        ${ROOT}/hlring/rbuf.c)
#set(SOURCE_FILES
#        ${ROOT}/test.cpp)

#include_directories(${ROOT})

# 添加第三方库
#1. *.h
set(INC_DIR
        ${USR}/sdl2/2.0.9_1/include
        ${USR}/ffmpeg/4.1.4_1/include
        ${USR}/speex/1.2.0/include
        ${USR}/rtmpdump/2.4+20151223/include
        ${USR}/curl/7.65.3/include
        /Users/guochao/DBY_code/libyuv-ios/include)
include_directories(${INC_DIR})
#2. lib
set(LINK_DIR
        ${USR}/sdl2/2.0.9_1/lib
        ${USR}/ffmpeg/4.1.4_1/lib
        ${USR}/speex/1.2.0/lib
        ${USR}/rtmpdump/2.4+20151223/lib
        ${USR}/curl/7.65.3/lib
        /Users/guochao/DBY_code/libyuv-ios/lib)

link_directories(${LINK_DIR})

add_executable(${EXE_TARGET_NAME} ${SOURCE_FILES})

target_link_libraries(${EXE_TARGET_NAME} libSDL2.a libavutil.a libavcodec.a libavutil.a libswscale.a libavformat.a libspeex.a librtmp.a libyuv.a libcurl.a)
