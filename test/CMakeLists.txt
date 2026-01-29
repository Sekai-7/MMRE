cmake_minimum_required(VERSION 3.22)

project(MMRE)

add_executable(mmretest 
    TimelineCacheTest.cc
    CacheNodePoolTest.cc
    SharedMemoryManagerTest.cc
    MultiResourceCheckpointManagerTest.cc
    ../code/src/TimelineCache.cc
    ../code/src/CacheNodePool.cc
    ../code/src/SharedMemoryManager.cc
    ../code/src/MultiResourceCheckpointManager.cc
)

target_include_directories(mmretest PRIVATE ../code/inc)

find_package(Threads REQUIRED)
find_package(GTest REQUIRED)

target_compile_features(mmretest PRIVATE cxx_std_17)

target_compile_options(mmretest PRIVATE -Wall -Wextra -Werror -fsanitize=thread)

target_link_options(mmretest PRIVATE -fsanitize=thread)

target_link_libraries(mmretest PRIVATE GTest::gtest_main Threads::Threads)

gtest_discover_tests(mmretest)