add_library(perf_utils INTERFACE)
target_include_directories(perf_utils
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)


add_subdirectory(parser)
add_subdirectory(order_manager)
add_subdirectory(risk)
add_subdirectory(builder)

