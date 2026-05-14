#-------------------BIST--------------------

add_executable(builder_ouch_bist_perf
    builder_ouch_bist_perf.cpp
)
target_link_libraries(builder_ouch_bist_perf
    PRIVATE
    perf_utils
    builder_ouch
    test_data
    project_options
)
target_compile_features(builder_ouch_bist_perf PRIVATE cxx_std_23)


#-------------------NASDAQ-------------------

add_executable(builder_ouch_nasdaq_perf
    builder_ouch_nasdaq_perf.cpp
)
target_link_libraries(builder_ouch_nasdaq_perf
    PRIVATE
    perf_utils
    builder_ouch
    test_data
    project_options
)
target_compile_features(builder_ouch_nasdaq_perf PRIVATE cxx_std_23)
