add_executable(parser_fix_parseFunc_bench
    parser_fix_parseFunc_bench.cpp
)
target_link_libraries(parser_fix_parseFunc_bench
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main 
    parser_fix
    test_data
    project_options
    bench_utils
)
target_compile_features(parser_fix_parseFunc_bench PRIVATE cxx_std_23)


add_executable(parser_fix_nextFixMsgFunc_bench
    parser_fix_nextFixMsgFunc_bench.cpp
)
target_link_libraries(parser_fix_nextFixMsgFunc_bench
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main 
    parser_fix
    test_data
    project_options
    bench_utils
)
target_compile_features(parser_fix_nextFixMsgFunc_bench PRIVATE cxx_std_23)


add_executable(parser_fix_bench
    parser_fix_bench.cpp
)
target_link_libraries(parser_fix_bench
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main 
    test_data
    project_options
    network
    session
    builder_fix
    parser_fix
    protocol
    bench_utils
    Threads::Threads
)
target_compile_features(parser_fix_bench PRIVATE cxx_std_23)
