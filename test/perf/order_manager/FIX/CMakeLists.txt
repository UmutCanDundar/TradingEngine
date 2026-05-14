add_executable(order_manager_fix_perf
    order_manager_fix_perf.cpp
)
target_link_libraries(order_manager_fix_perf 
    PRIVATE
    perf_utils
    test_data
    project_options
    order_management
    market
    core
)
target_compile_features(order_manager_fix_perf PRIVATE cxx_std_23)
