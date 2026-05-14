file(GLOB PROTOCOL_SRC CONFIGURE_DEPENDS *.cpp)

add_library(protocol STATIC ${PROTOCOL_SRC})

add_subdirectory(parsers/FIX)
add_subdirectory(parsers/ITCH)
add_subdirectory(parsers/OUCH)
add_subdirectory(builders/FIX)
add_subdirectory(builders/OUCH)
add_subdirectory(types/FIX)
add_subdirectory(types/ITCH)
add_subdirectory(types/OUCH)

target_include_directories(protocol
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(protocol
    PUBLIC
        core
        network
        session 
        parser_itch
        parser_ouch
        builder_ouch
        boost_headers
        types_fix
        database
    PRIVATE
        parser_fix
        utils
        builder_fix
        order_management
        project_options
)

target_compile_features(protocol PRIVATE cxx_std_23)


