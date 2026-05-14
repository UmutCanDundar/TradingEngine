add_executable(builder_fix_bench
    builder_fix_bench.cpp
)
target_link_libraries(builder_fix_bench
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main 
    builder_fix
    test_data
    project_options
    bench_utils
)
target_compile_features(builder_fix_bench PRIVATE cxx_std_23)
