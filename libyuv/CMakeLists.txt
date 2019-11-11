cmake_minimum_required(VERSION 3.7)
project(libyuv)

# 支持c++11
SET(CMAKE_C_COMPILER g++)
add_compile_options(-std=c++14)

# 提高编译警告级别
add_definitions(-Wall)

set(EXE_TARGET_NAME libyuv)

set(ROOT ${CMAKE_CURRENT_LIST_DIR})
# message("${ROOT}")
set(USR /usr/local/Cellar)

set(SOURCE_FILES ${ROOT}/yuv.cpp ${ROOT}/yuvscale.cpp)

# 添加第三方库
#1. *.h
set(INC_DIR
        /Users/guochao/DBY_code/libyuv-ios/include)
include_directories(${INC_DIR})
#2. lib
set(LINK_DIR
        /Users/guochao/DBY_code/libyuv-ios/lib)

link_directories(${LINK_DIR})

add_executable(${EXE_TARGET_NAME} ${SOURCE_FILES})

target_link_libraries(${EXE_TARGET_NAME} libyuv.a)
