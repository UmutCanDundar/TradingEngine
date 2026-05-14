add_executable(risk_bench
    risk_bench.cpp
)
target_link_libraries(risk_bench 
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
    bench_utils 
    test_data
    project_options
    order_management
    market
    core
    risk
)
target_compile_features(risk_bench PRIVATE cxx_std_23)




