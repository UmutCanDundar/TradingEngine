add_executable(parser_fix_1pkt_1msg_unit
    parser_fix_1pkt_1msg_unit.cpp
)

target_link_libraries(parser_fix_1pkt_1msg_unit 
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    parser_fix
    test_data
    project_options
)

target_compile_features(parser_fix_1pkt_1msg_unit PRIVATE cxx_std_23)
gtest_discover_tests(parser_fix_1pkt_1msg_unit)

add_executable(parser_fix_npkt_nmsg_unit
    parser_fix_npkt_nmsg_unit.cpp
)
target_link_libraries(parser_fix_npkt_nmsg_unit
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    parser_fix
    test_data
    project_options
    network
    session
    builder_fix
    protocol
)
target_compile_features(parser_fix_npkt_nmsg_unit PRIVATE cxx_std_23)

# target_compile_options(parser_fix_npkt_nmsg_unit PRIVATE
   
#     $<$<NOT:$<CONFIG:Release>>:-fsanitize=address,undefined>
# )

# target_link_options(parser_fix_npkt_nmsg_unit PRIVATE
#     $<$<NOT:$<CONFIG:Release>>:-fsanitize=address,undefined>
# )

gtest_discover_tests(parser_fix_npkt_nmsg_unit)