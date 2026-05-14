file(GLOB ITCH_SRC CONFIGURE_DEPENDS *.cpp)

add_library(parser_itch STATIC ${ITCH_SRC})

target_include_directories(parser_itch
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}   
)

target_link_libraries(parser_itch
    PUBLIC 
        types_itch
        core
        network
    PRIVATE
        utils
        project_options
)

target_compile_features(parser_itch PRIVATE cxx_std_23)

