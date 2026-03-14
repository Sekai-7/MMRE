add_library(mmre_engine_core STATIC "")
target_include_directories(mmre_engine_core PUBLIC include)
target_link_libraries(mmre_engine_core PUBLIC mmre_common mmre_memory)
