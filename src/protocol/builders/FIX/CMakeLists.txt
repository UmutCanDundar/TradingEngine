file(GLOB FIX_SRC CONFIGURE_DEPENDS *.cpp)

add_library(builder_fix STATIC ${FIX_SRC})

target_include_directories(builder_fix
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}  
)

target_link_libraries(builder_fix
    PUBLIC
        core 
        types_fix
        session
    PRIVATE 
        utils
        project_options
)

target_compile_features(builder_fix PRIVATE cxx_std_23)



