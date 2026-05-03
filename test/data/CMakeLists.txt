
add_library(test_data INTERFACE)

target_include_directories(test_data
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(test_data
    INTERFACE
        network
        core
        generated_headers
        ab_hash
)

