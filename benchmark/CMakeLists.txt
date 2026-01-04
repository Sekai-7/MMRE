cmake_minimum_required(VERSION 3.22)

project(MMRE)

add_executable(mmreBenchmark TimelineCacheBenchmark.cc)

find_package(Threads REQUIRED)
find_package(benchmark REQUIRED)

target_compile_features(mmreBenchmark PRIVATE cxx_std_17)

target_compile_options(mmreBenchmark PRIVATE -Wall -Wextra -Werror)

target_link_libraries(mmreBenchmark PRIVATE Threads::Threads benchmark::benchmark_main)