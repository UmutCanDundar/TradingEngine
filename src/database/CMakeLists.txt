file(GLOB DATABASE_SRC CONFIGURE_DEPENDS *.cpp)

add_library(database STATIC ${DATABASE_SRC})

target_include_directories(database
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(database
    PUBLIC 
        core
        clickhouse-cpp-lib
    PRIVATE
        project_options
)

target_compile_features(database PRIVATE cxx_std_23)

set(MARKER_FILE "${CMAKE_BINARY_DIR}/setup_sql.done")

add_custom_command(
    OUTPUT ${MARKER_FILE}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tables/setup_sql.sh ${MARKER_FILE}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tables/setup_sql.sh
        ${PROJECT_SOURCE_DIR}/src/database/tables/fixtable.sql
        ${PROJECT_SOURCE_DIR}/src/database/tables/itchtable.sql
        #${PROJECT_SOURCE_DIR}/src/database/tables/ouchtable.sql
        ${PROJECT_SOURCE_DIR}/src/database/tables/ordertable.sql
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_custom_target(SetupSqlTables ALL
    DEPENDS ${MARKER_FILE}
)