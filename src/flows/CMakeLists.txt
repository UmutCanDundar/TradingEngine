file(GLOB FLOWS_SRC CONFIGURE_DEPENDS *.cpp)

add_library(flows STATIC ${FLOWS_SRC})

target_include_directories(flows
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(flows
    PRIVATE
        database
        network
        order_management
        protocol
        risk
        utils
        Threads::Threads
        project_options
)

target_compile_features(flows PRIVATE cxx_std_23)

