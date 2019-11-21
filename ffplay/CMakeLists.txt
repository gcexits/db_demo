cmake_minimum_required(VERSION 3.7)
project(ffplay)

# 支持c++14
SET(CMAKE_C_COMPILER g++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

# 提高编译警告级别
add_definitions(-Wall)

set(EXE_TARGET_NAME ffplay)

set(ROOT ${CMAKE_CURRENT_LIST_DIR})
set(USR /usr/local/Cellar)

set(SOURCE_FILES
        ${ROOT}/main.cpp
        ${ROOT}/HttpClient.cpp
        ${ROOT}/HttpFile.cpp
        ${ROOT}/SDLPlayer.cpp
        ${ROOT}/H264Decoder.cpp
        ${ROOT}/AudioDecoder.cpp
        ${ROOT}/Demuxer.cpp
        ${ROOT}/../src/hlring/RingBuffer.cpp
        ${ROOT}/../src/hlring/rbuf.c
        ${ROOT}/Video.cpp
        ${ROOT}/Audio.cpp
        ${ROOT}/Media.cpp
        ${ROOT}/PacketQueue.cpp
        ${ROOT}/FrameQueue.cpp
        ${ROOT}/VideoDisplay.cpp)

#include_directories(${ROOT})

# 添加第三方库
#1. *.h
set(INC_DIR
        ${USR}/sdl2/2.0.9_1/include
        ${USR}/ffmpeg/4.1.4_1/include
        ${USR}/curl/7.65.3/include)
include_directories(${INC_DIR})
#2. lib
set(LINK_DIR
        ${USR}/sdl2/2.0.9_1/lib
        ${USR}/ffmpeg/4.1.4_1/lib
        ${USR}/curl/7.65.3/lib)

link_directories(${LINK_DIR})

add_executable(${EXE_TARGET_NAME} ${SOURCE_FILES})

target_link_libraries(${EXE_TARGET_NAME} libSDL2.a libavutil.a libavcodec.a libavutil.a libswscale.a libswresample.a libavformat.a libcurl.a)
