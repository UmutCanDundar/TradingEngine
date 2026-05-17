add_executable(engine_Rx_bench
    engine_Rx_bench.cpp
)
target_link_libraries(engine_Rx_bench 
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
    bench_utils
    test_data
    project_options
    TradingEngine 
)
target_compile_features(engine_Rx_bench PRIVATE cxx_std_23)


add_executable(engine_Tx_bench
    engine_Tx_bench.cpp
)
target_link_libraries(engine_Tx_bench 
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
    bench_utils
    test_data
    project_options
    TradingEngine 
)
target_compile_features(engine_Tx_bench PRIVATE cxx_std_23)