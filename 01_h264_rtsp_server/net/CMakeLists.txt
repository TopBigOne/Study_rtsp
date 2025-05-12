cmake_minimum_required(VERSION 3.22)
# 声明net模块
project(net)
file(GLOB NET_SOURCES "*.cpp" "poller/*.cpp")
file(GLOB NET_HEADERS "*.h" "poller/*.h")


# 创建静态库（也可以改为SHARED创建动态库）
add_library(net STATIC ${NET_SOURCES} ${NET_HEADERS})

target_include_directories(net
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/poller
        )

# 添加依赖库（如需要）
target_link_libraries(net
        PUBLIC
        base  # 依赖base
        pthread  # 示例依赖

        )

