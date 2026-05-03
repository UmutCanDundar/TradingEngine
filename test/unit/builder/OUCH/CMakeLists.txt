add_executable(builder_ouch_bist_unit
    builder_ouch_bist_unit.cpp
)
target_link_libraries(builder_ouch_bist_unit 
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    test_data
    project_options
    session
    builder_ouch
)
target_compile_features(builder_ouch_bist_unit PRIVATE cxx_std_23)
gtest_discover_tests(builder_ouch_bist_unit)

add_executable(builder_ouch_nasdaq_unit
    builder_ouch_nasdaq_unit.cpp
)
target_link_libraries(builder_ouch_nasdaq_unit 
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    test_data
    project_options
    session
    builder_ouch
)
target_compile_features(builder_ouch_nasdaq_unit PRIVATE cxx_std_23)
gtest_discover_tests(builder_ouch_nasdaq_unit)
