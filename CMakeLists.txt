cmake_minimum_required(VERSION 3.7)
project(test)

# 支持c++11
SET(CMAKE_C_COMPILER g++)
add_compile_options(-std=c++11)

# 提高编译警告级别
add_definitions(-Wall)

set(EXE_TARGET_NAME test)

set(ROOT ${CMAKE_CURRENT_LIST_DIR})
# message("${ROOT}")
set(USR /usr/local/Cellar)
set(THREAD /Users/guochao/DBY_code/1_mobileSDK_livplay/dby-client-core-sdk/OnlineVersion/root)
message("${THREAD}")

if (WIN32)
    set(CROSS_PATH win32)
elseif (APPLE)
    set(CROSS_PATH darwin)
endif()

set(RTMP_MODE tcpsocket)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/bin)

set(SOURCE_FILES ${ROOT}/flvparse.cpp)

# 添加第三方库
include_directories(${USR}/sdl2/2.0.9/include)
include_directories(${USR}/ffmpeg/4.1.1/include)
include_directories(${USR}/speex/1.2.0/include/speex)
include_directories(${USR}/rtmpdump/2.4+20151223/include/librtmp)

set(LOCAL_LINK_LIBRARIES
        ${USR}/sdl2/2.0.9/lib/libSDL2.a
        ${USR}/ffmpeg/4.1.1/lib/tmp/libavcodec.a
        ${USR}/ffmpeg/4.1.1/lib/tmp/libavutil.a
        ${USR}/speex/1.2.0/lib/libspeex.a
        ${USR}/rtmpdump/2.4+20151223/lib/librtmp.a)

add_executable(${EXE_TARGET_NAME} ${SOURCE_FILES})

target_link_libraries(${EXE_TARGET_NAME} ${LOCAL_LINK_LIBRARIES})