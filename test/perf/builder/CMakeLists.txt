add_executable(builder_dispatch_perf
    builder_dispatch_perf.cpp
)
target_link_libraries(builder_dispatch_perf 
    PRIVATE
    perf_utils
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
target_compile_features(builder_dispatch_perf PRIVATE cxx_std_23)

add_subdirectory(FIX)
add_subdirectory(OUCH)

