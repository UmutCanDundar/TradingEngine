add_executable(risk_update_perf
    risk_update_perf.cpp
)
target_link_libraries(risk_update_perf 
    PRIVATE
    perf_utils
    test_data
    project_options
    order_management
    market
    core
    risk
)
target_compile_features(risk_update_perf PRIVATE cxx_std_23)

add_executable(risk_check_perf
    risk_check_perf.cpp
)
target_link_libraries(risk_check_perf 
    PRIVATE
    perf_utils
    test_data
    project_options
    order_management
    market
    core
    risk
)
target_compile_features(risk_check_perf PRIVATE cxx_std_23)



