# EXECUTABLE
add_executable(TradingEngine
    main.cpp
    TradingEngine.cpp
)

set_property(TARGET TradingEngine PROPERTY INTERPROCEDURAL_OPTIMIZATION
    $<$<CONFIG:Release>:TRUE>
)

target_link_libraries(TradingEngine  
    PRIVATE
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
        project_options
)

target_compile_features(TradingEngine PRIVATE cxx_std_23)

add_dependencies(TradingEngine GeneratedFiles SetupSqlTables)

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