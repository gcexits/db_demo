cmake_minimum_required(VERSION 3.7)
project(airPlay)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")

set(EXE_TARGET_NAME airPlay)

set(ROOT ${CMAKE_CURRENT_LIST_DIR}/airPlay)
set(SRC ${CMAKE_CURRENT_LIST_DIR}/duobei)

set(AIRPLAY
        ${SRC}/airPlay/server.cpp)

set(PARSER
        ${SRC}/parser/PacketParser.cpp)

if (WIN32)
elseif(APPLE)
    include_directories(${ROOT}/include)

    set(LINK_DIR
            ${ROOT}/lib)
    link_directories(${LINK_DIR})
endif ()

add_executable(${EXE_TARGET_NAME} ${AIRPLAY})

target_link_libraries(${EXE_TARGET_NAME} libplay-lib.a)