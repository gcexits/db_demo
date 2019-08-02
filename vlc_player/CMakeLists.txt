cmake_minimum_required(VERSION 3.7)
project(test)

# 支持c++11
SET(CMAKE_C_COMPILER g++)
add_compile_options(-std=c++14)

# 提高编译警告级别
add_definitions(-Wall)

set(EXE_TARGET_NAME test)

set(ROOT ${CMAKE_CURRENT_LIST_DIR})
# message("${ROOT}")
set(USR /usr/local/Cellar)

set(SOURCE_FILES ${ROOT}/vlcPlayer.cpp)

# 添加第三方库
#1. *.h
set(INC_DIR
        /Applications/VLC.app/Contents/MacOS/include)
include_directories(${INC_DIR})
#2. lib
set(LINK_DIR
        /Applications/VLC.app/Contents/MacOS/lib)

link_directories(${LINK_DIR})

add_executable(${EXE_TARGET_NAME} ${SOURCE_FILES})

target_link_libraries(${EXE_TARGET_NAME} libvlc.dylib)
