cmake_minimum_required(VERSION 3.7)
project(ffmpeg_capture)

# 支持c++14
SET(CMAKE_C_COMPILER g++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

# 提高编译警告级别
add_definitions(-Wall)

set(EXE_TARGET_NAME ffmpeg_capture)

set(ROOT ${CMAKE_CURRENT_LIST_DIR})
set(USR /usr/local/Cellar)

set(SOURCE_FILES
        ${ROOT}/main.cpp
        ${ROOT}/VideoRecorder.cpp
        ../utils/Time.cpp
        ../h264_parse/PacketParser.cpp
        ../h264_parse/RtmpSender.cpp
        ../h264_parse/H264Encoder.cpp
        ../h264_parse/VideoBuffer.cpp)

#include_directories(${ROOT})

# 添加第三方库
#1. *.h
set(INC_DIR
        ${USR}/sdl2/2.0.9_1/include
        ${USR}/ffmpeg/4.1.4_1/include
        ${USR}/rtmpdump/2.4+20151223/include)
include_directories(${INC_DIR})
#2. lib
set(LINK_DIR
        ${USR}/sdl2/2.0.9_1/lib
        ${USR}/ffmpeg/4.1.4_1/lib
        ${USR}/rtmpdump/2.4+20151223/lib)

link_directories(${LINK_DIR})

add_executable(${EXE_TARGET_NAME} ${SOURCE_FILES})

target_link_libraries(${EXE_TARGET_NAME} libavutil.a libavcodec.a libswscale.a libswresample.a libavformat.a libavdevice.a librtmp.a libSDL2.a)
