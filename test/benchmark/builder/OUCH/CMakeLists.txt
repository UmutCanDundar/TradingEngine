#-------------------BIST--------------------

add_executable(builder_ouch_bist_bench
    builder_ouch_bist_bench.cpp
)
target_link_libraries(builder_ouch_bist_bench
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main 
    builder_ouch
    test_data
    project_options
    bench_utils
)
target_compile_features(builder_ouch_bist_bench PRIVATE cxx_std_23)
add_test(NAME builder_ouch_bist_bench COMMAND builder_ouch_bist_bench)


#-------------------NASDAQ-------------------

add_executable(builder_ouch_nasdaq_bench
    builder_ouch_nasdaq_bench.cpp
)
target_link_libraries(builder_ouch_nasdaq_bench
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main 
    builder_ouch
    test_data
    project_options
    bench_utils
)
target_compile_features(builder_ouch_nasdaq_bench PRIVATE cxx_std_23)
add_test(NAME builder_ouch_nasdaq_bench COMMAND builder_ouch_nasdaq_bench)
