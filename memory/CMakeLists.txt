# sekai-7/mmre/MMRE-ai/memory/CMakeLists.txt
add_library(mmre_memory STATIC 
    src/SharedMemoryManager.cpp
    src/SharedMemoryPtr.cpp
)

target_include_directories(mmre_memory PUBLIC include)
target_link_libraries(mmre_memory PUBLIC mmre_common)

# 必须链接实时库(rt)以支持 shm_open/shm_unlink，以及 uuid 库
target_link_libraries(mmre_memory PRIVATE rt uuid)