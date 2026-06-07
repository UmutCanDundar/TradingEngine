file(GLOB UTILS_SRC CONFIGURE_DEPENDS *.cpp)

add_library(utils STATIC ${UTILS_SRC})

target_include_directories(utils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)   

target_link_libraries(utils
    PUBLIC
        Threads::Threads
        spdlog_headers
        generated_headers
    PRIVATE 
        project_options
)

target_compile_features(utils PRIVATE cxx_std_23)

add_subdirectory(CustomMap)