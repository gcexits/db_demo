cmake_minimum_required(VERSION 3.7)
project(virtualCamera)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")

set(EXE_TARGET_NAME virtualCamera)
set(TARGET_LINK_LIBRARIES)

if (WIN32)
    include_directories(/Users/guochao/DBY_code/virtual_camera/windows/vcam-mic-src/libVCamMicSource)

    set(TARGET_LINK_LIBRARIES
            /Users/guochao/DBY_code/virtual_camera/windows/vcam-mic-src/lib/static/nanomsg.lib
            /Users/guochao/DBY_code/virtual_camera/windows/vcam-mic-src/dist/x86/libVCamMicSource.lib)
elseif(APPLE)
endif ()

add_executable(${EXE_TARGET_NAME} ${CMAKE_CURRENT_LIST_DIR}/main.cpp)

target_link_libraries(${EXE_TARGET_NAME} ${TARGET_LINK_LIBRARIES})