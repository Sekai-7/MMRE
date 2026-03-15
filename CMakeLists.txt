cmake_minimum_required(VERSION 3.16)

project(timeline_resource_engine
        VERSION 0.1.0
        LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_SHARED_LIBS "Build shared libraries by default" ON)

# Global include directories for public headers and common types
include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/common"
)

add_subdirectory(server)
add_subdirectory(client)

