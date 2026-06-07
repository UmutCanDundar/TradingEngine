file(GLOB OMS_SRC CONFIGURE_DEPENDS *.cpp)

add_library(order_management STATIC ${OMS_SRC})

target_include_directories(order_management
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}      
)

target_link_libraries(order_management
    PUBLIC
        core
        ab_hash
        ab_flat_hash_map
        custom_map
        boost_headers
        database
        fillers
    PRIVATE
        market
        project_options
)

target_compile_features(order_management PRIVATE cxx_std_23)

add_subdirectory(fillers)
