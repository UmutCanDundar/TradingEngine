add_executable(order_manager_bench
    order_manager_bench.cpp
)
target_link_libraries(order_manager_bench 
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
target_compile_features(order_manager_bench PRIVATE cxx_std_23)


add_subdirectory(FIX)
add_subdirectory(ITCH)
add_subdirectory(OUCH)


