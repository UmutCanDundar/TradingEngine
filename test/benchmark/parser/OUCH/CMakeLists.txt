#-------------------BIST--------------------

add_executable(parser_ouch_bist_parseFunc_bench
    parser_ouch_bist_parseFunc_bench.cpp
)
target_link_libraries(parser_ouch_bist_parseFunc_bench
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main 
    parser_ouch
    test_data
    project_options
    bench_utils
)
target_compile_features(parser_ouch_bist_parseFunc_bench PRIVATE cxx_std_23)
add_test(NAME parser_ouch_bist_parseFunc_bench COMMAND parser_ouch_bist_parseFunc_bench)

#------------------------------------------------

add_executable(parser_ouch_bist_bench
    parser_ouch_bist_bench.cpp
)
target_link_libraries(parser_ouch_bist_bench
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
    parser_ouch
    Threads::Threads
)
target_compile_features(parser_ouch_bist_bench PRIVATE cxx_std_23)

#------------------------------------------------

add_executable(parser_ouch_bist_pktHandler_bench
    parser_ouch_bist_pktHandler_bench.cpp
)
target_link_libraries(parser_ouch_bist_pktHandler_bench
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main 
    test_data
    project_options
    network
    builder_fix
    session
    bench_utils
    Threads::Threads
)
target_compile_features(parser_ouch_bist_pktHandler_bench PRIVATE cxx_std_23)


#-------------------NASDAQ-------------------

add_executable(parser_ouch_nasdaq_parseFunc_bench
    parser_ouch_nasdaq_parseFunc_bench.cpp
)
target_link_libraries(parser_ouch_nasdaq_parseFunc_bench
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main 
    parser_ouch
    test_data
    project_options
    bench_utils
)
target_compile_features(parser_ouch_nasdaq_parseFunc_bench PRIVATE cxx_std_23)
