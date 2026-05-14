add_executable(parser_dispatch_perf
    parser_dispatch_perf.cpp
)
target_link_libraries(parser_dispatch_perf 
    PRIVATE
    perf_utils
    test_data
    project_options
    network
    session
    builder_fix
    parser_fix
    protocol
)
target_compile_features(parser_dispatch_perf PRIVATE cxx_std_23)

add_subdirectory(FIX)
add_subdirectory(ITCH)
add_subdirectory(OUCH)

