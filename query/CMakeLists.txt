add_library(mmre_query STATIC "")
target_include_directories(mmre_query PUBLIC include)
target_link_libraries(mmre_query PUBLIC mmre_common mmre_engine_core mmre_memory)
