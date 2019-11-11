cmake_minimum_required(VERSION 3.7)
project(json)

# 支持c++11
SET(CMAKE_C_COMPILER g++)
add_compile_options(-std=c++14)

# 提高编译警告级别
add_definitions(-Wall)

set(EXE_TARGET_NAME test)

set(ROOT ${CMAKE_CURRENT_LIST_DIR})
# message("${ROOT}")
set(USR /usr/local/Cellar)

#set(SOURCE_FILES ${ROOT}/H264HardDecode.cpp ${ROOT}/HardwareContext.cpp ${ROOT}/yuvscale.cpp)
#set(SOURCE_FILES ${ROOT}/simplest_ffmpeg_decoder.cpp)
set(SOURCE_FILES ${ROOT}/json.cpp ${ROOT}/RapidJsonWrapper.hpp)

# 添加第三方库
#1. *.h
set(INC_DIR
        /Users/guochao/DBY_code/2_mobile_发版/dby-client-core-sdk/OnlineVersion/root/third_party)
include_directories(${INC_DIR})
#2. lib
set(LINK_DIR
        )

link_directories(${LINK_DIR})

add_executable(${EXE_TARGET_NAME} ${SOURCE_FILES})

target_link_libraries(${EXE_TARGET_NAME})
