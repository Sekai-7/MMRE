cmake_minimum_required(VERSION 3.16)
project(TimelineCache)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/inc)

find_package(benchmark REQUIRED)

add_subdirectory(src/benchmark)
add_subdirectory(src/test)