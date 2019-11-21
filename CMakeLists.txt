cmake_minimum_required(VERSION 3.7)
project(client_testing)

# 支持c++14
set(CMAKE_C_COMPILER g++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

# 提高编译警告级别
add_definitions(-Wall)

set(EXE_TARGET_NAME client_testing)

set(ROOT ${CMAKE_CURRENT_LIST_DIR}/src)
set(USR /usr/local/Cellar)

set(SOURCE_FILES
        ${ROOT}/client_testing/main.cpp)

set(COMMON_SOURCE_FILES
        ${ROOT}/utils/Optional.cpp
        ${ROOT}/utils/Time.cpp

        ${ROOT}/codec/H264Decoder.cpp
        ${ROOT}/codec/AACDecoder.cpp
        ${ROOT}/codec/SpeexDecoder.cpp

        ${ROOT}/hlring/RingBuffer.cpp
        ${ROOT}/hlring/rbuf.c

        ${ROOT}/display/SDLPlayer.cpp

        ${ROOT}/network/HttpClient.cpp
        ${ROOT}/network/HttpFile.cpp

        ${ROOT}/demux/Demuxer.cpp

        ${ROOT}/offline/FlvParse.cpp)

set(TESTING_FILES
        ${ROOT}/client_testing/ffmpeg_curl.cpp
        ${ROOT}/client_testing/flv_player.cpp)

#include_directories(${ROOT})

# 添加第三方库
#1. *.h
set(INC_DIR
        ${USR}/sdl2/2.0.9_1/include
        ${USR}/ffmpeg/4.1.4_1/include
        ${USR}/curl/7.65.3/include
        ${USR}/rtmpdump/2.4+20151223/include
        ${USR}/speex/1.2.0/include)
include_directories(${INC_DIR})
#2. lib
set(LINK_DIR
        ${USR}/sdl2/2.0.9_1/lib
        ${USR}/ffmpeg/4.1.4_1/lib
        ${USR}/curl/7.65.3/lib
        ${USR}/rtmpdump/2.4+20151223/lib
        ${USR}/speex/1.2.0/lib)

link_directories(${LINK_DIR})

add_executable(${EXE_TARGET_NAME} ${SOURCE_FILES} ${COMMON_SOURCE_FILES} ${TESTING_FILES})

target_link_libraries(${EXE_TARGET_NAME} libSDL2.a libavutil.a libavcodec.a libavutil.a libswscale.a libswresample.a libavformat.a libcurl.a librtmp.a libspeex.a)
