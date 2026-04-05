file(GLOB OUCH_SRC CONFIGURE_DEPENDS *.cpp)

add_library(parser_ouch STATIC ${OUCH_SRC})

target_include_directories(parser_ouch
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(parser_ouch
    PUBLIC
        types_ouch
        core
    PRIVATE 
        utils
        project_options
)

target_compile_features(parser_ouch PRIVATE cxx_std_23)

