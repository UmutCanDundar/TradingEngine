file(GLOB RISK_SRC CONFIGURE_DEPENDS *.cpp)

add_library(risk STATIC ${RISK_SRC})

target_include_directories(risk
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(risk
    PUBLIC
        core
        utils
        ab_flat_hash_map
        boost_headers
    PRIVATE
        order_management
        market
        lohmann_json
        project_options
)

target_compile_features(risk PRIVATE cxx_std_23)


