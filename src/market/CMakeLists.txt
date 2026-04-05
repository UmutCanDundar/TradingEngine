file(GLOB MARKET_SRC CONFIGURE_DEPENDS *.cpp)

add_library(market STATIC ${MARKET_SRC})

target_include_directories(market
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(market
    PUBLIC
        core
        utils
        ab_btree
    PRIVATE 
        project_options
)

target_compile_features(market PRIVATE cxx_std_23)

