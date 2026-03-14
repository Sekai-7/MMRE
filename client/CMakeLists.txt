add_library(mmre_client SHARED "") # .cpp files for the SDK will be added here
target_include_directories(mmre_client PUBLIC include)
target_link_libraries(mmre_client PRIVATE mmre_common)
