add_executable(order_manager_unit
    order_manager_unit.cpp
)
target_link_libraries(order_manager_unit 
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    test_data
    project_options
    order_management
    market
    core
)
target_compile_features(order_manager_unit PRIVATE cxx_std_23)
gtest_discover_tests(order_manager_unit)




