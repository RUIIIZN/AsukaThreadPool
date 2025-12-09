cmake_minimum_required(VERSION 3.10)
project(AsukaThreadPool VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置可执行文件输出目录为项目根目录下的 bin 文件夹
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# 选项：是否构建压力测试
option(BUILD_STRESS_TESTS "Build stress test executables" ON)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ThreadPool/include
    ${CMAKE_CURRENT_SOURCE_DIR}/SyncQueue
)

# 源文件
set(THREAD_POOL_SOURCES
    ThreadPool/src/FixedThreadPool.cc
    ThreadPool/src/CacheThreadPool.cc
    ThreadPool/src/WorkStealingThreadPool.cc
)

# 创建线程池so库 (已修改库名称)
add_library(AsukaThreadPool STATIC ${THREAD_POOL_SOURCES})

# 设置库的头文件目录（用于安装）
target_include_directories(AsukaThreadPool PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ThreadPool/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/SyncQueue>
    $<INSTALL_INTERFACE:include>
)

install(DIRECTORY ThreadPool/include/ DESTINATION include/ThreadPool
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY SyncQueue/ DESTINATION include/SyncQueue
    FILES_MATCHING PATTERN "*.hpp"
)

if(BUILD_STRESS_TESTS)
    add_executable(stress_fixed test/stress_fixed.cc)
    target_link_libraries(stress_fixed AsukaThreadPool)

    add_executable(stress_cache test/stress_cache.cc)
    target_link_libraries(stress_cache AsukaThreadPool)

    add_executable(stress_workstealing test/stress_workstealing.cc)
    target_link_libraries(stress_workstealing AsukaThreadPool)
endif()