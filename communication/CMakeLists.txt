add_library(mmre_communication STATIC "")
target_include_directories(mmre_communication PUBLIC include)
target_link_libraries(mmre_communication PUBLIC mmre_common)
