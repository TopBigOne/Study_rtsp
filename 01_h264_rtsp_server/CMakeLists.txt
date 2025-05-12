cmake_minimum_required(VERSION 3.22)
project(01_h264_rtsp_server)

set(CMAKE_CXX_STANDARD 11)
add_subdirectory(base)
add_subdirectory(net)


# 主程序源文件
set(MAIN_SOURCE main.cpp)

add_executable(01_h264_rtsp_server ${MAIN_SOURCE})

target_link_libraries(
        01_h264_rtsp_server
        PRIVATE
        net       # 链接net子模块
)


