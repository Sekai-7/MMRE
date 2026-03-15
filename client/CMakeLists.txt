cmake_minimum_required(VERSION 3.16)

set(TL_CLIENT_SOURCES
    timeline_client.cpp
)

add_library(timelineclient_obj OBJECT ${TL_CLIENT_SOURCES})

add_library(timelineclient_shared SHARED $<TARGET_OBJECTS:timelineclient_obj>)
add_library(timelineclient_static STATIC $<TARGET_OBJECTS:timelineclient_obj>)

set_target_properties(timelineclient_shared PROPERTIES
    OUTPUT_NAME timelineclient
)

set_target_properties(timelineclient_static PROPERTIES
    OUTPUT_NAME timelineclient
)

target_compile_features(timelineclient_shared PRIVATE cxx_std_17)
target_compile_features(timelineclient_static PRIVATE cxx_std_17)

target_include_directories(timelineclient_shared
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../common"
)

target_include_directories(timelineclient_static
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../common"
)

