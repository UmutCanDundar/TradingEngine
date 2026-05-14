#-------------------BIST--------------------

add_executable(parser_ouch_bist_perf
    parser_ouch_bist_perf.cpp
)
target_link_libraries(parser_ouch_bist_perf
    PRIVATE
    perf_utils
    test_data
    project_options
    network
    session
    builder_fix
    parser_fix
    protocol
    parser_ouch
    Threads::Threads
)
target_compile_features(parser_ouch_bist_perf PRIVATE cxx_std_23)

#-------------------NASDAQ-------------------

add_executable(parser_ouch_nasdaq_parseFunc_perf
    parser_ouch_nasdaq_parseFunc_perf.cpp
)
target_link_libraries(parser_ouch_nasdaq_parseFunc_perf
    PRIVATE
    perf_utils
    parser_ouch
    test_data
    project_options
)
target_compile_features(parser_ouch_nasdaq_parseFunc_perf PRIVATE cxx_std_23)
