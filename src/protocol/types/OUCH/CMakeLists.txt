add_library(types_ouch INTERFACE)

target_include_directories(types_ouch
    INTERFACE
         ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(types_ouch 
    INTERFACE 
        generated_headers
)