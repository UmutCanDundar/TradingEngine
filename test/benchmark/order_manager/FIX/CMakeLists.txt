add_executable(order_manager_fix_single_bench
    order_manager_fix_single_bench.cpp
)
target_link_libraries(order_manager_fix_single_bench 
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
    bench_utils 
    test_data
    project_options
    order_management
    market
    core
)
target_compile_features(order_manager_fix_single_bench PRIVATE cxx_std_23)


add_executable(order_manager_fix_bench
    order_manager_fix_bench.cpp
)
target_link_libraries(order_manager_fix_bench 
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
    bench_utils 
    test_data
    project_options
    order_management
    market
    core
)
target_compile_features(order_manager_fix_bench PRIVATE cxx_std_23)
