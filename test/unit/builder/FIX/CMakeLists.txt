add_executable(builder_fix_unit
    builder_fix_unit.cpp
)

target_link_libraries(builder_fix_unit 
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    builder_fix
    test_data
    project_options
    session
    core
)

target_compile_features(builder_fix_unit PRIVATE cxx_std_23)
gtest_discover_tests(builder_fix_unit)
