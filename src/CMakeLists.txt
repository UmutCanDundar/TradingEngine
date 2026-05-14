#LIBRARY
add_library(TradingEngine
    TradingEngine.cpp
)

set_property(TARGET TradingEngine PROPERTY INTERPROCEDURAL_OPTIMIZATION
    $<$<CONFIG:Release>:TRUE>
)

target_include_directories(TradingEngine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(TradingEngine  
    PUBLIC
        utils
        core
        database
        network
        session
        protocol
        market
        order_management
        risk
        #strategy
        flows
        builder_fix
        parser_fix
        project_options
)
target_compile_features(TradingEngine PRIVATE cxx_std_23)
add_dependencies(TradingEngine GeneratedFiles SetupSqlTables)

#EXECUTABLE
add_executable(main
    main.cpp
)
target_link_libraries(main
    PUBLIC
    TradingEngine
    utils
)
target_compile_features(main PRIVATE cxx_std_23)

# SUBMODULES
add_subdirectory(utils)
add_subdirectory(core)
add_subdirectory(database)
add_subdirectory(market)
add_subdirectory(order_management)
#add_subdirectory(strategy)
add_subdirectory(risk)
add_subdirectory(session)
add_subdirectory(network)
add_subdirectory(protocol)
add_subdirectory(flows)