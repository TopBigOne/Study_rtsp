cmake_minimum_required(VERSION 3.22)
# 声明net模块
project(base)
file(GLOB BASE_SOURCES "*.cpp")
file(GLOB BASE_HEADERS "*.h")

# 创建静态库（也可以改为SHARED创建动态库）
add_library(base STATIC ${BASE_SOURCES} ${BASE_HEADERS})

target_include_directories(base
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        )

# 添加依赖库（如需要）
target_link_libraries(base
        PUBLIC
        pthread  # 示例依赖
        )

