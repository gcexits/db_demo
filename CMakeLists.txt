cmake_minimum_required(VERSION 3.7)
project(dby-client-tool)

set(CMAKE_C_COMPILER g++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

set(LIB_TARGET_NAME dby-electron-core)
set(EXE_TARGET_NAME dby-client-tool)


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

set(ROOT /Users/guochao/DBY_code/2_mobile_发版/dby-client-core-sdk/OnlineVersion/root)
set(USER ${CMAKE_CURRENT_LIST_DIR}/src)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/tool/cmake/")

#写文件
#add_definitions(-DWRITE_PCM_DATA_TO_FILE_FOR_ANALYSIS)

if (NightlyBuild)
    add_definitions(-DNIGHTLY_BUILD)
endif ()

if (DEVELOPMENT_OPTION)
    add_definitions(-DUSE_PRINTF_OUTPUT_LOG_FOR_DEVELOPMENT)
    add_definitions(-DENABLE_WRITE_LOG)
    add_definitions(-DENABLE_DEV_LOG)
    add_definitions(-DTURN_OFF_LOG_UPLOAD_CALLING)
endif()

if (HLS_STREAMING_OPTION)
    add_definitions(-DDBY_CLIENT_TOOL_FOR_HLS_STREAMING_TESTING)
endif ()

if (DISABEL_LIBSOL_OPTION)
    add_definitions(-DDISABEL_LIBSOL_STREAMING_CONNECTION)
endif ()

set(RTMP_MODE unisocket)
add_definitions(-DRTMP_SUPPORT_UNISOCKET)

if (WIN32)
    set(CROSS_PATH win32)
elseif (APPLE)
    set(CROSS_PATH darwin)
endif()

if (MSVC)
    add_definitions(-DNOMINMAX)
else()
    set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf")
    set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/lib)


set(CLIENT_LIVING_FILES
        ${USER}/client_testing/main.cpp
        ${USER}/client_testing/ffmpeg_curl.cpp
        ${USER}/client_testing/flv_player.cpp
        ${USER}/client_testing/send_h264.cpp
        ${USER}/client_testing/send_speex.cpp
        ${USER}/client_testing/deal_yuv.cpp
        ${USER}/client_testing/ffmpeg_capture.cpp

        ${USER}/client_testing/audio_mix.cpp
        ${USER}/client_testing/demux_audio_mix.cpp
        ${USER}/client_testing/simplest_ffmpeg_video_filter.cpp

        ${USER}/client_testing/json.cpp
        ${USER}/client_testing/c++_test.cpp)

set(LIVE_SOURCE_FILES
        )

set(PLAYBACK_SOURCE_FILES
        ${USER}/offline/FlvParse.cpp)

set(COMMON_SOURCE_FILES
        ${USER}/utils/Optional.cpp
        ${USER}/utils/Time.cpp
        ${USER}/utils/Yuvscale.cpp

        ${USER}/codec/H264Decoder.cpp
        ${USER}/codec/H264Encoder.cpp
        ${USER}/codec/AACDecoder.cpp
        ${USER}/codec/SpeexDecoder.cpp
        ${USER}/codec/SpeexEncoder.cpp
        ${USER}/codec/OpusDecoder.cpp
        ${USER}/codec/OpusEncoder.cpp
        ${USER}/codec/AudioBuffer.cpp
        ${USER}/codec/VideoBuffer.cpp
        ${USER}/codec/VideoRecorder.cpp

        ${USER}/hlring/RingBuffer.cpp
        ${USER}/hlring/rbuf.c

        ${USER}/display/SDLPlayer.cpp

        ${USER}/network/HttpClient.cpp
        ${USER}/network/HttpFile.cpp

        ${USER}/demux/Demuxer.cpp


        ${USER}/stream/RtmpSender.cpp

        ${USER}/common/PacketParser.cpp)

set(SOUND_TOUCH_FILES
        ${ROOT}/third_party/soundtouch/cpu_detect_x86.cpp
        ${ROOT}/third_party/soundtouch/mmx_optimized.cpp
        ${ROOT}/third_party/soundtouch/sse_optimized.cpp
        ${ROOT}/third_party/soundtouch/AAFilter.cpp
        ${ROOT}/third_party/soundtouch/BPMDetect.cpp
        ${ROOT}/third_party/soundtouch/FIFOSampleBuffer.cpp
        ${ROOT}/third_party/soundtouch/FIRFilter.cpp
        ${ROOT}/third_party/soundtouch/InterpolateCubic.cpp
        ${ROOT}/third_party/soundtouch/InterpolateLinear.cpp
        ${ROOT}/third_party/soundtouch/InterpolateShannon.cpp
        ${ROOT}/third_party/soundtouch/PeakFinder.cpp
        ${ROOT}/third_party/soundtouch/RateTransposer.cpp
        ${ROOT}/third_party/soundtouch/SoundTouch.cpp
        ${ROOT}/third_party/soundtouch/TDStretch.cpp)

include_directories(/Users/guochao/DBY_code/libyuv-ios/include)
include_directories(/usr/local/Cellar/speex/1.2.0/include/speex)
include_directories(/usr/local/Cellar/rtmpdump/2.4+20151223/include)
include_directories(${ROOT}/third_party/opus/${CROSS_PATH}/include)

include_directories(${ROOT}/include/${CROSS_PATH})
include_directories(${ROOT}/third_party)
include_directories(${ROOT}/third_party/soundtouch)
include_directories(${ROOT}/include/${RTMP_MODE})
include_directories(${ROOT}/third_party/sdl2/${CROSS_PATH}/include)
include_directories(${ROOT}/third_party/ffmpeg/${CROSS_PATH}/include)

add_executable(${EXE_TARGET_NAME} ${SOUND_TOUCH_FILES} ${COMMON_SOURCE_FILES} ${LIVE_SOURCE_FILES} ${SPEEX} ${SPEEXDSP} ${LIBRTMP} ${CLIENT_LIVING_FILES} ${PLAYBACK_SOURCE_FILES})

## 测试
#add_subdirectory(/Users/guochao/DBY_code/ff_test/src/client_testing)


# Windows 处理
if (WIN32)
elseif(APPLE)
    message(STATUS ">>> DBY LiveStreaming SDK for macOS, CMAKE_GENERATOR is ${CMAKE_GENERATOR}")

    set(CMAKE_CXX_FLAGS_RELEASE  " -O2 -DNDEBUG ")

    set(SDL_LINK_LIBRARIES
            ${ROOT}/third_party/sdl2/${CROSS_PATH}/lib/libSDL2.a

            "-framework CoreAudio"
            "-framework AudioToolBox"
            "-framework CoreFoundation"
            "-framework CoreGraphics"
            "-framework OpenGL"
            "-framework CoreImage"

            "-framework CoreVideo"
            "-framework ForceFeedback"
            "-framework IOKit"
            "-framework Carbon"
            "-framework AppKit"
            )

    if (${RTMP_MODE} STREQUAL "unisocket")
        set(TARGET_LINK_LIBRARIES
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libavcodec.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libavdevice.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libavfilter.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libavformat.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libpostproc.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libavutil.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libswscale.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libswresample.a

                ${ROOT}/lib/${CROSS_PATH}/liblzma.a

                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libvpx.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libaom.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libtheora.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libtheoraenc.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libtheoradec.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libfdk-aac.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libopencore-amrnb.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libopencore-amrwb.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libogg.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libaom.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libxvidcore.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libvorbis.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libx264.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libx265.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libmp3lame.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libvorbisenc.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libvorbisfile.a
                ${ROOT}/third_party/ffmpeg/${CROSS_PATH}/lib/libvidstab.a

                ${ROOT}/third_party/opus/${CROSS_PATH}/lib/libopus.a
                /usr/local/Cellar/speex/1.2.0/lib/libspeex.a
                /usr/local/Cellar/rtmpdump/2.4+20151223/lib/librtmp.a
                /Users/guochao/DBY_code/libyuv-ios/lib/libyuv.a

                ${ROOT}/lib/${CROSS_PATH}/libcrypto.a
                ${ROOT}/lib/${CROSS_PATH}/libcurl.a
                ${ROOT}/lib/${CROSS_PATH}/libssl.a
                "-framework Security"
                "-framework AVFoundation"

                "-framework CoreFoundation"
                "-framework CoreMedia"
                "-framework CoreVideo"

                "-framework VideoToolBox"
                "-framework AudioToolBox"
                "-framework VideoDecodeAcceleration"

                "-framework LDAP"
                bz2
                iconv
                z)
    endif()

endif()

target_link_libraries(${EXE_TARGET_NAME} ${TARGET_LINK_LIBRARIES} ${SDL_LINK_LIBRARIES})




