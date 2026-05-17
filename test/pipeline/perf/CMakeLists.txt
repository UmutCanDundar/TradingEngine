add_executable(engine_Rx_perf
    engine_Rx_perf.cpp
)
target_link_libraries(engine_Rx_perf 
    PRIVATE
    perf_utils
    test_data
    project_options
    TradingEngine 
)
target_compile_features(engine_Rx_perf PRIVATE cxx_std_23)


add_executable(engine_Tx_perf
    engine_Tx_perf.cpp
)
target_link_libraries(engine_Tx_perf 
    PRIVATE
    perf_utils
    test_data
    project_options
    TradingEngine 
)
target_compile_features(engine_Tx_perf PRIVATE cxx_std_23)