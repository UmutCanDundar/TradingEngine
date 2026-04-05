add_library(bench_utils INTERFACE)
target_include_directories(bench_utils
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(FIX)
