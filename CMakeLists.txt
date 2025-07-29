cmake_minimum_required(VERSION 3.16)
project(MarketData)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES CMAKE_CONFIGURE_DEPENDS 
      ${CMAKE_SOURCE_DIR}/src/*.cpp 
    )

# Python3 bul
find_package(Python3 COMPONENTS Interpreter REQUIRED)

find_package(fmt REQUIRED)

find_package(Threads REQUIRED)


# .h dosyasını generate eden komut
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/include/GeneratedErrorMap.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/generate/GenerateErrorMap.py
    DEPENDS ${CMAKE_SOURCE_DIR}/generate/GenerateErrorMap.py
)
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/include/GeneratedIpPort.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/generate/GenerateIpPort.py
    DEPENDS ${CMAKE_SOURCE_DIR}/generate/GenerateIpPort.py
)
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/include/GeneratedITCHMessages.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/generate/GenerateITCHMessages.py
    DEPENDS ${CMAKE_SOURCE_DIR}/generate/GenerateITCHMessages.py
)
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/include/GeneratedSBEMessages.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/generate/GenerateSBEMessages.py
    DEPENDS ${CMAKE_SOURCE_DIR}/generate/GenerateSBEMessages.py
)
add_custom_target(GeneratedFiles ALL
    DEPENDS ${CMAKE_SOURCE_DIR}/include/GeneratedErrorMap.h
    DEPENDS ${CMAKE_SOURCE_DIR}/include/GeneratedIpPort.h
    DEPENDS ${CMAKE_SOURCE_DIR}/include/GeneratedITCHMessages.h
    DEPENDS ${CMAKE_SOURCE_DIR}/include/GeneratedSBEMessages.h
)

set(MARKER_FILE "${CMAKE_BINARY_DIR}/setup_sql.done")
add_custom_command(
    OUTPUT ${MARKER_FILE}
    COMMAND ${CMAKE_SOURCE_DIR}/setup_sql.sh
    DEPENDS
        ${CMAKE_SOURCE_DIR}/setup_sql.sh
        ${CMAKE_SOURCE_DIR}/tables/fixtable.sql
        ${CMAKE_SOURCE_DIR}/tables/itchtable.sql
        ${CMAKE_SOURCE_DIR}/tables/sbetable.sql
        ${CMAKE_SOURCE_DIR}/tables/ordertable.sql
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
add_custom_target(SetupSqlTables ALL
    DEPENDS ${MARKER_FILE}
)

# main.cpp ve diğer kaynak dosyaları
add_executable(MarketData ${SOURCES})

# include path ve bağımlılık
target_include_directories(MarketData PRIVATE include)

target_link_libraries(MarketData PRIVATE fmt::fmt absl::flat_hash_map absl::hash clickhouse-cpp-lib Threads::Threads)

target_compile_options(MarketData PRIVATE 
    -mavx2 
    -Wno-deprecated-enum-enum-conversion
)

add_dependencies(MarketData GeneratedFiles SetupSqlTables)

add_subdirectory(third-party/absl/abseil-cpp)
add_subdirectory(third-party/clickhouse/clickhouse-cpp)




