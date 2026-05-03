add_executable(parser_itch_bist_unit
    parser_itch_bist_unit.cpp
)
target_link_libraries(parser_itch_bist_unit 
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    parser_itch
    test_data
    project_options
)
target_compile_features(parser_itch_bist_unit PRIVATE cxx_std_23)
gtest_discover_tests(parser_itch_bist_unit)

add_executable(parser_itch_nasdaq_unit
    parser_itch_nasdaq_unit.cpp
)
target_link_libraries(parser_itch_nasdaq_unit 
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    parser_itch
    test_data
    project_options
)
target_compile_features(parser_itch_nasdaq_unit PRIVATE cxx_std_23)
gtest_discover_tests(parser_itch_nasdaq_unit)

