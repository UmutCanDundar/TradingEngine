

add_executable(parser_fix_perf
    parser_fix_perf.cpp
)

target_link_libraries(parser_fix_perf
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main 
    parser_fix
    test_data
    project_options
    bench_utils
)

target_compile_features(parser_fix_perf PRIVATE cxx_std_23)
target_compile_options(parser_fix_perf PRIVATE -w)

add_test(NAME parser_fix_perf COMMAND parser_fix_perf)