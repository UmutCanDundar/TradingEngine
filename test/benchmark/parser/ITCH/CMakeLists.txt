#-------------------BIST--------------------

add_executable(parser_itch_bist_parseFunc_bench
    parser_itch_bist_parseFunc_bench.cpp
)
target_link_libraries(parser_itch_bist_parseFunc_bench
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main 
    parser_itch
    test_data
    project_options
    bench_utils
)
target_compile_features(parser_itch_bist_parseFunc_bench PRIVATE cxx_std_23)

#------------------------------------------------

add_executable(parser_itch_bist_bench
    parser_itch_bist_bench.cpp
)
target_link_libraries(parser_itch_bist_bench
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
    parser_itch
    Threads::Threads
)
target_compile_features(parser_itch_bist_bench PRIVATE cxx_std_23)

#-------------------NASDAQ-------------------

add_executable(parser_itch_nasdaq_parseFunc_bench
    parser_itch_nasdaq_parseFunc_bench.cpp
)
target_link_libraries(parser_itch_nasdaq_parseFunc_bench
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main 
    parser_itch
    test_data
    project_options
    bench_utils
)
target_compile_features(parser_itch_nasdaq_parseFunc_bench PRIVATE cxx_std_23)

#---------------------------------------------

add_executable(parser_itch_nasdaq_bench
    parser_itch_nasdaq_bench.cpp
)
target_link_libraries(parser_itch_nasdaq_bench
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
    parser_itch
    Threads::Threads
)
target_compile_features(parser_itch_nasdaq_bench PRIVATE cxx_std_23)
