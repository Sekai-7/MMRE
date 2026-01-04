cmake_minimum_required(VERSION 3.22)

project(MMRE)

add_executable(mmre ./src/TimelineCache.cc ./src/CacheNodePool.cc)

find_package(Threads REQUIRED)

target_compile_features(mmre PRIVATE cxx_std_17)

target_compile_options(mmre PRIVATE -Wall -Wextra -Werror)

target_link_libraries(mmre PRIVATE Threads::Threads)

target_include_directories(mmre PRIVATE ./inc)