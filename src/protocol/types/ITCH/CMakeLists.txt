add_library(types_itch INTERFACE)

target_include_directories(types_itch
    INTERFACE
         ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(types_itch 
    INTERFACE 
        generated_headers
)