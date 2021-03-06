cmake_minimum_required(VERSION 3.7)
project(sdl_player)

# 支持c++14
SET(CMAKE_C_COMPILER g++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

# 提高编译警告级别
add_definitions(-Wall)

set(EXE_TARGET_NAME sdl_player)

set(ROOT ${CMAKE_CURRENT_LIST_DIR})
set(USR /usr/local/Cellar)

set(SOURCE_FILES
        ${ROOT}/main.cpp
        ${ROOT}/Audio.cpp
        ${ROOT}/FrameQueue.cpp
        ${ROOT}/Media.cpp
        ${ROOT}/PacketQueue.cpp
        ${ROOT}/Video.cpp
        ${ROOT}/VideoDisplay.cpp)

#include_directories(${ROOT})

# 添加第三方库
#1. *.h
set(INC_DIR
        ${USR}/sdl2/2.0.10/include
        ${USR}/ffmpeg/4.2.1_2/include)
include_directories(${INC_DIR})
#2. lib
set(LINK_DIR
        ${USR}/sdl2/2.0.10/lib
        ${USR}/ffmpeg/4.2.1_2/lib)

link_directories(${LINK_DIR})

add_executable(${EXE_TARGET_NAME} ${SOURCE_FILES})

target_link_libraries(${EXE_TARGET_NAME} libSDL2.a libavutil.a libavcodec.a libavutil.a libswscale.a libswresample.a libavformat.a)
