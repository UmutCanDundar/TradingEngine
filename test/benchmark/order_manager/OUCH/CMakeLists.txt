#-------------------BIST---------------------

add_executable(order_manager_ouch_bist_single_bench
    order_manager_ouch_bist_single_bench.cpp
)
target_link_libraries(order_manager_ouch_bist_single_bench 
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
target_compile_features(order_manager_ouch_bist_single_bench PRIVATE cxx_std_23)

#-----------------------------------------------

add_executable(order_manager_ouch_bist_bench
    order_manager_ouch_bist_bench.cpp
)
target_link_libraries(order_manager_ouch_bist_bench 
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
target_compile_features(order_manager_ouch_bist_bench PRIVATE cxx_std_23)


#-------------------NASDAQ---------------------

add_executable(order_manager_ouch_nasdaq_single_bench
    order_manager_ouch_nasdaq_single_bench.cpp
)
target_link_libraries(order_manager_ouch_nasdaq_single_bench 
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
target_compile_features(order_manager_ouch_nasdaq_single_bench PRIVATE cxx_std_23)

#-----------------------------------------------

add_executable(order_manager_ouch_nasdaq_bench
    order_manager_ouch_nasdaq_bench.cpp
)
target_link_libraries(order_manager_ouch_nasdaq_bench 
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
target_compile_features(order_manager_ouch_nasdaq_bench PRIVATE cxx_std_23)