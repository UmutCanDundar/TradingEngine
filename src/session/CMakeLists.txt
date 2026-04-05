file(GLOB SESSION_SRC CONFIGURE_DEPENDS *.cpp)

add_library(session STATIC ${SESSION_SRC})

target_include_directories(session
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(session
    PUBLIC 
        core
        utils
    PRIVATE
        network
        builder_fix
        lohmann_json
        project_options
)

target_compile_features(session PRIVATE cxx_std_23)

add_executable(sess 
    sess.main.cpp
)

target_link_libraries(sess
    PRIVATE 
        session 
    
)

target_compile_features(sess PRIVATE cxx_std_23)
