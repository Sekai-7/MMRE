cmake_minimum_required(VERSION 3.16)

add_executable(resource-server
    main.cpp
    server_ipc.cpp
)

target_compile_features(resource-server PRIVATE cxx_std_17)

target_include_directories(resource-server
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../common"
)

if(UNIX)
    target_link_libraries(resource-server PRIVATE pthread)
endif()

