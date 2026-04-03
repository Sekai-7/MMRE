add_executable(tl_query_latest_demo
    tl_query_latest_demo.cpp
)

target_link_libraries(tl_query_latest_demo PRIVATE timelineclient_shared)