cmake_minimum_required(VERSION 3.22)
project(01_h264_rtsp_server)

set(CMAKE_CXX_STANDARD 11)
add_subdirectory(base)
add_subdirectory(net)


# 主程序源文件
set(MAIN_SOURCE main.cpp)

add_executable(01_h264_rtsp_server ${MAIN_SOURCE})
# 包含子模块头文件
target_include_directories(01_h264_rtsp_server
        PRIVATE
        ${CMAKE_SOURCE_DIR}/base
        ${CMAKE_SOURCE_DIR}/net
)

target_link_libraries(
        01_h264_rtsp_server
        PRIVATE
        net       # 链接net子模块
        base  # 必须添加，因为net依赖base
)


