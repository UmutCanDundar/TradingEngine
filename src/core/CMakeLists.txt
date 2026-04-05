file(GLOB CORE_SRC CONFIGURE_DEPENDS *.cpp)

add_library(core STATIC ${CORE_SRC})

target_include_directories(core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core
    PUBLIC 
        types_fix
        types_itch
        types_ouch
    PRIVATE 
        generated_headers
        project_options
)

target_compile_features(core PRIVATE cxx_std_23)

