add_library(mmre_manager STATIC "")
target_include_directories(mmre_manager PUBLIC include)
target_link_libraries(mmre_manager PUBLIC mmre_engine_core mmre_ingestion mmre_communication mmre_memory)
