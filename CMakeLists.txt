cmake_minimum_required(VERSION 3.7)
project(client-tool)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")

set(EXE_TARGET_NAME client-tool)

# 提高编译警告级别
add_definitions(-Wall)

set(ROOT ${CMAKE_CURRENT_LIST_DIR}/root)
set(USER /usr/local/Cellar)

if (WIN32)
    set(CROSS_PATH win)
elseif (APPLE)
    set(CROSS_PATH mac)
endif()

set(PLAYBACK_SOURCE_FILES
        ${ROOT}/src_code/offline/FlvParse.cpp)

set(COMMON_SOURCE_FILES
        ${ROOT}/src_code/utils/Optional.cpp
        ${ROOT}/src_code/utils/Time.cpp
        ${ROOT}/src_code/utils/Yuvscale.cpp

        ${ROOT}/src_code/codec/H264Decoder.cpp
        ${ROOT}/src_code/codec/H264Encoder.cpp
        ${ROOT}/src_code/codec/AudioDecoder.cpp
        ${ROOT}/src_code/codec/SpeexDecoder.cpp
        ${ROOT}/src_code/codec/SpeexEncoder.cpp
        ${ROOT}/src_code/codec/OpusDecoder.cpp
        ${ROOT}/src_code/codec/OpusEncoder.cpp
        ${ROOT}/src_code/codec/AudioBuffer.cpp
        ${ROOT}/src_code/codec/VideoBuffer.cpp
        ${ROOT}/src_code/codec/VideoRecorder.cpp
        ${ROOT}/src_code/codec/HardwareContext.cpp

        ${ROOT}/src_code/hlring/RingBuffer.cpp
        ${ROOT}/src_code/hlring/rbuf.c

        ${ROOT}/src_code/display/SDLPlayer.cpp

        ${ROOT}/src_code/network/HttpClient.cpp
        ${ROOT}/src_code/network/HttpFile.cpp

        ${ROOT}/src_code/demux/Demuxer.cpp

        ${ROOT}/src_code/stream/RtmpSender.cpp

        ${ROOT}/src_code/common/PacketParser.cpp)

# Windows 处理
if (WIN32)
# MacOs 处理
elseif(APPLE)
    message(STATUS ">>> DBY LiveStreaming SDK for macOS, CMAKE_GENERATOR is ${CMAKE_GENERATOR}")

    set(CMAKE_CXX_FLAGS_RELEASE  " -O2 -DNDEBUG ")

    # 添加第三方库
    #1. *.h
    include_directories(${USER}/sdl2/2.0.10/include)
    include_directories(${USER}/ffmpeg/4.2.1_2/include)
    include_directories(${USER}/curl/7.65.3/include)
    include_directories(${USER}/rtmpdump/2.4+20151223_1/include)
    include_directories(${USER}/speex/1.2.0/include)
    include_directories(${USER}/rapidjson/1.1.0/include)
    include_directories(${ROOT}/third_party/libyuv/include)
    include_directories(${ROOT}/third_party/opus/${CROSS_PATH}/include)

    #2. lib
    set(LINK_DIR
            ${USER}/sdl2/2.0.10/lib
            ${USER}/ffmpeg/4.2.1_2/lib
            ${USER}/curl/7.65.3/lib
            ${USER}/rtmpdump/2.4+20151223_1/lib
            ${USER}/speex/1.2.0/lib
            ${ROOT}/third_party/libyuv/lib
            ${ROOT}/third_party/opus/${CROSS_PATH}/lib)
    link_directories(${LINK_DIR})

endif()

include_directories(${ROOT}/client_testing)

add_executable(${EXE_TARGET_NAME} ${COMMON_SOURCE_FILES} ${PLAYBACK_SOURCE_FILES})

target_link_libraries(${EXE_TARGET_NAME} client_library)

add_subdirectory(${ROOT}/client_testing)

target_link_libraries(${EXE_TARGET_NAME} libSDL2.a libavutil.a libavcodec.a libavutil.a libswscale.a libswresample.a libavformat.a libavfilter.a libavdevice.a libcurl.a librtmp.a libspeex.a libyuv.a libopus.a)




