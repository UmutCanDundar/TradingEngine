add_executable(parser_dispatch_unit
    parser_dispatch_unit.cpp
)
target_link_libraries(parser_dispatch_unit 
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    test_data
    project_options
    network
    session
    parser_fix
    builder_fix
    protocol
)
target_compile_features(parser_dispatch_unit PRIVATE cxx_std_23)

gtest_discover_tests(parser_dispatch_unit)

add_subdirectory(FIX)
add_subdirectory(ITCH)
add_subdirectory(OUCH)



