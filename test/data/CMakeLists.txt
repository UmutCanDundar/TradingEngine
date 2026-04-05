file(GLOB DATASET CONFIGURE_DEPENDS *.cpp)

add_library(test_data STATIC ${DATASET})

target_include_directories(test_data
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(test_data
    PUBLIC
        network
)

target_compile_features(test_data PRIVATE cxx_std_23)