add_library(types_fix INTERFACE)

target_include_directories(types_fix
    INTERFACE
         ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(types_fix 
    INTERFACE 
        boost_headers
)