add_executable(network_unit
    network_unit.cpp
)
target_link_libraries(network_unit 
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    test_data
    project_options
    core
    order_management
    market
    builder_fix
    protocol
    utils
    session
    parser_fix
    network
)

target_compile_features(network_unit PRIVATE cxx_std_23)
gtest_discover_tests(network_unit)
