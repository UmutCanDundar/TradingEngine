file(GLOB FIX_SRC CONFIGURE_DEPENDS *.cpp)

add_library(parser_fix STATIC ${FIX_SRC})

target_include_directories(parser_fix
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(parser_fix
    PUBLIC
        utils
        ab_btree
        types_fix
    PRIVATE 
        network
        session
        project_options
)

target_compile_features(parser_fix PRIVATE cxx_std_23)


