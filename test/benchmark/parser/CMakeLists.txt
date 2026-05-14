add_executable(parser_dispatch_bench
    parser_dispatch_bench.cpp
)
target_link_libraries(parser_dispatch_bench 
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
    bench_utils
    test_data
    project_options
    network
    session
    parser_fix
    builder_fix
    protocol
)
target_compile_features(parser_dispatch_bench PRIVATE cxx_std_23)

add_subdirectory(FIX)
add_subdirectory(ITCH)
add_subdirectory(OUCH)

