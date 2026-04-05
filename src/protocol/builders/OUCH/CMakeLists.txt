file(GLOB OUCH_SRC CONFIGURE_DEPENDS *.cpp)

add_library(builder_ouch STATIC ${OUCH_SRC})

target_include_directories(builder_ouch
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(builder_ouch
    PUBLIC 
        core
        utils
        session
    PRIVATE 
        project_options
)

target_compile_features(builder_ouch PRIVATE cxx_std_23)

