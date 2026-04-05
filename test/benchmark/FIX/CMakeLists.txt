add_executable(parser_fix_bench
    parser_fix_bench.cpp
)

target_link_libraries(parser_fix_bench
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main 
    parser_fix
    test_data
    project_options
    bench_utils
)

target_compile_features(parser_fix_bench PRIVATE cxx_std_23)

add_test(NAME parser_fix_bench COMMAND parser_fix_bench)

add_executable(parser_fix_dispatch_bench
    parser_fix_dispatch_bench.cpp
)
target_link_libraries(parser_fix_dispatch_bench
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main 
    test_data
    project_options
    network
    session
    builder_fix
    protocol
    bench_utils
    Threads::Threads
)
target_compile_features(parser_fix_dispatch_bench PRIVATE cxx_std_23)
target_compile_options(parser_fix_dispatch_bench PRIVATE -O0 -g)

add_test(NAME parser_fix_dispatch_bench COMMAND parser_fix_dispatch_bench)