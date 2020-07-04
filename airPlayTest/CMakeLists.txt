cmake_minimum_required(VERSION 3.7)
project(airPlay)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")

set(EXE_TARGET_NAME airPlay)
set(CLIENT ${CMAKE_CURRENT_LIST_DIR}/duobei)

#add_definitions(-DSAVEFILE)

include_directories(${CLIENT})

include_directories(${CLIENT}/third_party/ffmpeg/include)
include_directories(${CLIENT}/third_party/airplay/include)

if (WIN32)
elseif(APPLE)
    set(TARGET_LINK_LIBRARIES
            ${CLIENT}/third_party/ffmpeg/lib/mac/libavcodec.58.dylib
            ${CLIENT}/third_party/ffmpeg/lib/mac/libavutil.56.dylib
            ${CLIENT}/third_party/ffmpeg/lib/mac/libswscale.5.dylib
            ${CLIENT}/third_party/ffmpeg/lib/mac/libavfilter.7.dylib

            ${CLIENT}/third_party/airplay/lib/mac/libcrypto.a
            ${CLIENT}/third_party/airplay/lib/mac/libcurve25519.a
            ${CLIENT}/third_party/airplay/lib/mac/libed25519.a
            ${CLIENT}/third_party/airplay/lib/mac/libfdk-aac.a
            ${CLIENT}/third_party/airplay/lib/mac/libplay-lib.a
            ${CLIENT}/third_party/airplay/lib/mac/libplayfair.a
            ${CLIENT}/third_party/airplay/lib/mac/libplist.a)
endif ()

add_subdirectory(${CLIENT})

add_executable(${EXE_TARGET_NAME} ${CMAKE_CURRENT_LIST_DIR}/test.cpp)

target_link_libraries(${EXE_TARGET_NAME} ${TARGET_LINK_LIBRARIES} client_library)