add_library(mmre_memory STATIC "") # .cpp files will be added here
target_include_directories(mmre_memory PUBLIC include)
target_link_libraries(mmre_memory PUBLIC mmre_common)
