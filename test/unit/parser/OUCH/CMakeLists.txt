add_executable(parser_ouch_bist_1pkt_1msg_unit
    parser_ouch_bist_1pkt_1msg_unit.cpp
)
target_link_libraries(parser_ouch_bist_1pkt_1msg_unit 
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    parser_ouch
    test_data
    project_options
)
target_compile_features(parser_ouch_bist_1pkt_1msg_unit PRIVATE cxx_std_23)
gtest_discover_tests(parser_ouch_bist_1pkt_1msg_unit)

add_executable(parser_ouch_npkt_nmsg_unit
    parser_ouch_npkt_nmsg_unit.cpp
)
target_link_libraries(parser_ouch_npkt_nmsg_unit 
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
target_compile_features(parser_ouch_npkt_nmsg_unit PRIVATE cxx_std_23)
gtest_discover_tests(parser_ouch_npkt_nmsg_unit)

add_executable(parser_ouch_nasdaq_1pkt_1msg_unit
    parser_ouch_nasdaq_1pkt_1msg_unit.cpp
)
target_link_libraries(parser_ouch_nasdaq_1pkt_1msg_unit 
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    parser_ouch
    test_data
    project_options
)
target_compile_features(parser_ouch_nasdaq_1pkt_1msg_unit PRIVATE cxx_std_23)
gtest_discover_tests(parser_ouch_nasdaq_1pkt_1msg_unit)

