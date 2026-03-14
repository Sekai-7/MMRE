cmake_minimum_required(VERSION 3.10)
project(MultiModalResourceEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# [架构优化说明] 开启严格的编译警告，并强制链接 pthread 以支持底层的无锁与多线程架构
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -pthread")

# 添加各个子模块目录
add_subdirectory(common)
add_subdirectory(memory)
add_subdirectory(engine_core)
add_subdirectory(query)
add_subdirectory(ingestion)
add_subdirectory(communication)
add_subdirectory(manager)
add_subdirectory(client)
add_subdirectory(server)
