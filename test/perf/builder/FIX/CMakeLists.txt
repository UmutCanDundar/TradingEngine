add_executable(builder_fix_perf
    builder_fix_perf.cpp
)
target_link_libraries(builder_fix_perf
    PRIVATE
    perf_utils
    builder_fix
    test_data
    project_options
)
target_compile_features(builder_fix_perf PRIVATE cxx_std_23)
