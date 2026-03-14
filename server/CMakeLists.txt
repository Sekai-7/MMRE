add_executable(mmre_server "") # server main.cpp will be added here
target_link_libraries(mmre_server PRIVATE mmre_manager mmre_communication mmre_ingestion mmre_engine_core mmre_memory pthread)
