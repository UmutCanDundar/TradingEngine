

add_executable(parser_fix_perf
    parser_fix_perf.cpp
)

target_link_libraries(parser_fix_perf
    PRIVATE
    perf_utils
    parser_fix
    test_data
    project_options
    protocol
    builder_fix
)

target_compile_features(parser_fix_perf PRIVATE cxx_std_23)

