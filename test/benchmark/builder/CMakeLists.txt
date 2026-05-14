add_executable(builder_dispatch_bench
    builder_dispatch_bench.cpp
)
target_link_libraries(builder_dispatch_bench 
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
    bench_utils
    test_data
    project_options
    core
    order_management
    network
    session
    market
    protocol
    builder_fix
    parser_fix
)
target_compile_features(builder_dispatch_bench PRIVATE cxx_std_23)

add_subdirectory(FIX)
add_subdirectory(OUCH)

