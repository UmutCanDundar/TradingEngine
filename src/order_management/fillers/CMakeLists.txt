file(GLOB FILLERS_SRC CONFIGURE_DEPENDS *.cpp)

add_library(fillers STATIC ${FILLERS_SRC})

target_include_directories(fillers
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(fillers
    PUBLIC
        core
    PRIVATE
        parser_fix
        utils
        order_management
        ab_hash
        generated_headers
        project_options         
)

target_compile_features(fillers PRIVATE cxx_std_23)

