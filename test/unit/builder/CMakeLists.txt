add_executable(builder_dispatch_unit
    builder_dispatch_unit.cpp
)
target_link_libraries(builder_dispatch_unit 
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    test_data
    project_options
    core
    order_management
    network
    session
    market
    protocol
    builder_fix
)
target_compile_features(builder_dispatch_unit PRIVATE cxx_std_23)
gtest_discover_tests(builder_dispatch_unit)


add_subdirectory(FIX)
add_subdirectory(OUCH)