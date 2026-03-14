add_library(mmre_ingestion STATIC "")
target_include_directories(mmre_ingestion PUBLIC include)
target_link_libraries(mmre_ingestion PUBLIC mmre_common mmre_memory mmre_engine_core)
