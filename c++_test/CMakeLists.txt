cmake_minimum_required(VERSION 3.7)
project(c++_test)

# 支持c++14
SET(CMAKE_C_COMPILER g++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# 提高编译警告级别
add_definitions(-Wall)

set(EXE_TARGET_NAME _test)

set(ROOT ${CMAKE_CURRENT_LIST_DIR})
set(USR /usr/local/Cellar)

set(SOURCE_FILES
        ${ROOT}/main.cpp)

#include_directories(${ROOT})

# 添加第三方库
#1. *.h
set(INC_DIR
        )
include_directories(${INC_DIR})
#2. lib
set(LINK_DIR
        )

link_directories(${LINK_DIR})

add_executable(${EXE_TARGET_NAME} ${SOURCE_FILES})

target_link_libraries(${EXE_TARGET_NAME})
