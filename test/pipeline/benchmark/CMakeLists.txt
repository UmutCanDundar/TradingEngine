add_executable(engine_incoming_bench
    engine_incoming_bench.cpp
)
target_link_libraries(engine_incoming_bench 
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
    bench_utils
    test_data
    project_options
    TradingEngine 
   
)
target_compile_features(engine_incoming_bench PRIVATE cxx_std_23)


