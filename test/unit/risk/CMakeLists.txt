add_executable(risk_unit
    risk_unit.cpp
)
target_link_libraries(risk_unit 
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    test_data
    project_options
    order_management
    market
    core
    risk
)
target_compile_features(risk_unit PRIVATE cxx_std_23)
gtest_discover_tests(risk_unit)




