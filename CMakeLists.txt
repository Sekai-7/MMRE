cmake_minimum_required(VERSION 3.16)
project(TimelineCache)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SRC_LIST ${CMAKE_SOURCE_DIR}/*.cc)
include_directories(${CMAKE_SOURCE_DIR}/inc)

find_package(benchmark REQUIRED)
find_package(Threads REQUIRED)

add_executable(TimelineCache ${SRC_LIST})

target_link_libraries(TimelineCache
    PRIVATE
    benchmark::benchmark
    Threads::Threads 
)
