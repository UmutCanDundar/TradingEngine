
add_library(custom_spsc_queue INTERFACE)

target_include_directories(custom_spsc_queue
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)   

target_link_libraries(custom_spsc_queue
    INTERFACE
        project_options
        utils
)

target_compile_features(custom_spsc_queue INTERFACE cxx_std_23)


