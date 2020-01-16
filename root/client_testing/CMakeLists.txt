project(testing)

set(CLIENT_FILES
        ${ROOT}/client_testing/main.cpp
        ${ROOT}/client_testing/deal_yuv.cpp
        ${ROOT}/client_testing/mainPlayer.cpp

        ${ROOT}/client_testing/demux_audio_mix.cpp
        ${ROOT}/client_testing/simplest_ffmpeg_video_filter.cpp

        ${ROOT}/client_testing/json.cpp
        ${ROOT}/client_testing/c++_test.cpp

        ${ROOT}/client_testing/H264HardDecode.cpp)

add_library(client_library ${CLIENT_FILES})
target_include_directories(client_library PUBLIC ${PROJECT_SOURCE_DIR})