cmake_minimum_required(VERSION 3.7)
project(airPlay)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")

set(EXE_TARGET_NAME airPlay)
set(CLIENT ${CMAKE_CURRENT_LIST_DIR}/duobei)
set(USER /usr/local/Cellar)

if (WIN32)
elseif(APPLE)
    include_directories(${USER}/ffmpeg/4.2.1_2/include)
    include_directories(${CLIENT}/third_party/airplay/include)

    set(LINK_DIR
            ${USER}/ffmpeg/4.2.1_2/lib
            ${CLIENT}/third_party/airplay/lib)
    link_directories(${LINK_DIR})
endif ()

add_subdirectory(${CLIENT})

add_executable(${EXE_TARGET_NAME} ${CMAKE_CURRENT_LIST_DIR}/test.cpp)

target_link_libraries(${EXE_TARGET_NAME} libavcodec.a libavutil.a libswscale.a libavformat.a client_library play-lib crypto curve25519 ed25519 fdk-aac playfair plist)