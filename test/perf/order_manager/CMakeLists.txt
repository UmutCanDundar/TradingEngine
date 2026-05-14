add_executable(order_manager_perf
    order_manager_perf.cpp
)
target_link_libraries(order_manager_perf 
    PRIVATE
    perf_utils
    test_data
    project_options
    order_management
    market
    core
)
target_compile_features(order_manager_perf PRIVATE cxx_std_23)


add_subdirectory(FIX)
add_subdirectory(ITCH)
add_subdirectory(OUCH)


