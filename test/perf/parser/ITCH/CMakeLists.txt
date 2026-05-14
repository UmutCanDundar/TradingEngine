#-------------------BIST--------------------

add_executable(parser_itch_bist_perf
    parser_itch_bist_perf.cpp
)
target_link_libraries(parser_itch_bist_perf
    PRIVATE
    perf_utils
    test_data
    project_options
    network
    session
    builder_fix
    parser_fix
    protocol
    parser_itch
    Threads::Threads
)
target_compile_features(parser_itch_bist_perf PRIVATE cxx_std_23)

#-------------------NASDAQ-------------------

add_executable(parser_itch_nasdaq_perf
    parser_itch_nasdaq_perf.cpp
)
target_link_libraries(parser_itch_nasdaq_perf
    PRIVATE
    perf_utils
    test_data
    project_options
    network
    session
    builder_fix
    parser_fix
    protocol
    parser_itch
    Threads::Threads
)
target_compile_features(parser_itch_nasdaq_perf PRIVATE cxx_std_23)
