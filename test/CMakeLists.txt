include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)

FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.8.3
)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)    
set(BENCHMARK_FORCE_WERROR OFF CACHE BOOL "" FORCE) 

FetchContent_MakeAvailable(googletest)
FetchContent_MakeAvailable(benchmark)

target_compile_options(benchmark PRIVATE -w)
target_compile_options(benchmark_main PRIVATE -w)

enable_testing()

include(GoogleTest)

add_subdirectory(data)
add_subdirectory(unit)
add_subdirectory(benchmark)
add_subdirectory(perf)
add_subdirectory(pipeline)