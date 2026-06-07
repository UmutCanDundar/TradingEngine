
add_library(custom_map INTERFACE)

target_include_directories(custom_map
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)   

target_link_libraries(custom_map
    INTERFACE
        project_options
        utils
)

target_compile_features(custom_map INTERFACE cxx_std_23)


