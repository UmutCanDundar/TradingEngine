file(GLOB NETWORK_SRC CONFIGURE_DEPENDS *.cpp)

add_library(network STATIC ${NETWORK_SRC})

target_include_directories(network
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(network
    PUBLIC
        core
        utils
        session
        boost_headers
    PRIVATE
        project_options
)

target_compile_features(network PRIVATE cxx_std_23)



