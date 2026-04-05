cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0069 NEW)
project(TradingEngine LANGUAGES CXX)

#GLOBAL SETTINGS
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(USE_SYSTEM_DEPS "Use system dependencies if available" ON)

add_library(project_options INTERFACE)
target_compile_options(project_options INTERFACE
    -Wall -Wextra -Wno-pedantic 
    -O2
    -g
    -march=native
    -mtune=native
    -mavx2
    -mavx
    $<$<NOT:$<CONFIG:Release>>:-fno-omit-frame-pointer>
    $<$<NOT:$<CONFIG:Release>>:-fno-sized-deallocation>
    # $<$<NOT:$<CONFIG:Release>>:-fsanitize=address,undefined>
    # $<$<NOT:$<CONFIG:Release>>:-fno-optimize-sibling-calls>
    # $<$<NOT:$<CONFIG:Release>>:-fno-inline>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<AND:$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,13>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,14>>>:-Wno-array-bounds;-Wno-stringop-truncation;-Wno-stringop-overflow>
)

target_link_options(project_options INTERFACE
    # $<$<NOT:$<CONFIG:Release>>:-fsanitize=address,undefined>
)

#DEPENDENCIES
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(Threads REQUIRED)

#INCLUDE DIR
set(GENERATED_DIR ${PROJECT_SOURCE_DIR}/generated)
set(JSON_DIR ${PROJECT_SOURCE_DIR}/third-party/json/include)
set(BOOST_DIR ${PROJECT_SOURCE_DIR}/third-party/boost)
set(SPDLOG_DIR ${PROJECT_SOURCE_DIR}/third-party/spdlog)
set(ABSL_DIR ${PROJECT_SOURCE_DIR}/third-party/absl/abseil-cpp)
set(CLICKHOUSE_DIR ${PROJECT_SOURCE_DIR}/third-party/clickhouse/clickhouse-cpp)
set(FOLLY_DIR ${PROJECT_SOURCE_DIR}/third-party/folly)

#THIRD-PARTY
function(require_onlyheader_dep target_name include_dir)
        message(STATUS "Using bundled ${target_name}")

        if(NOT EXISTS "${include_dir}")
            message(FATAL_ERROR
                "${target_name} not found.\nRun ./setup.sh to install dependencies."
            )
        endif()
      
        target_include_directories(${target_name}
            SYSTEM INTERFACE
                ${include_dir}
        )
endfunction()

function(check_compiled_dep dep_name include_dir)
    message(STATUS "Using bundled ${dep_name}")
    if(NOT EXISTS "${include_dir}")
        message(FATAL_ERROR
            "${dep_name} not found.\nRun ./setup.sh to install dependencies."
        )
    endif()
endfunction()

#boost
if(USE_SYSTEM_DEPS)
    find_package(Boost 1.75 QUIET)
endif()

add_library(boost_headers INTERFACE)

if(Boost_FOUND AND TARGET Boost::boost)
        message(STATUS "Using system Boost")

        target_link_libraries(boost_headers
            INTERFACE
                Boost::boost
        )
else()
    require_onlyheader_dep(boost_headers ${BOOST_DIR})
endif()

#nlohmann-json
if(USE_SYSTEM_DEPS)
    find_package(nlohmann_json 3.12.0 QUIET)
endif()

add_library(lohmann_json INTERFACE)

if(nlohmann_json_FOUND AND TARGET nlohmann_json::nlohmann_json)
        message(STATUS "Using system nlohmann_json")

        target_link_libraries(lohmann_json
            INTERFACE
                nlohmann_json::nlohmann_json
        )
else()
    require_onlyheader_dep(lohmann_json ${JSON_DIR})
endif()

#spdlog
if(USE_SYSTEM_DEPS)
    find_package(spdlog QUIET)
endif()

add_library(spdlog_headers INTERFACE)

if(spdlog_FOUND AND TARGET spdlog::spdlog_header_only)
        message(STATUS "Using system spdlog")

        target_link_libraries(spdlog_headers 
            INTERFACE
                spdlog::spdlog_header_only
        )
else()
    require_onlyheader_dep(spdlog_headers ${SPDLOG_DIR})
endif()

#clickhouse
if(USE_SYSTEM_DEPS)
    find_package(clickhouse-cpp QUIET)
endif()

if(clickhouse-cpp_FOUND AND TARGET clickhouse-cpp-lib)
    message(STATUS "Using system clickhouse-cpp")
else()
    check_compiled_dep(clickhouse-cpp-lib ${CLICKHOUSE_DIR})
    add_subdirectory(${CLICKHOUSE_DIR})
    target_compile_options(clickhouse-cpp-lib PRIVATE -w)
endif()

#folly

if(USE_SYSTEM_DEPS)
    find_package(folly QUIET)
endif()
if(folly_FOUND AND TARGET folly)
    message(STATUS "Using system folly")
else()
    check_compiled_dep(folly ${FOLLY_DIR})
    set(CMAKE_CXX_FLAGS_BACKUP "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native")
    add_subdirectory(${FOLLY_DIR})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_BACKUP}")
    target_compile_options(folly PRIVATE -w -Wno-unused-result)
    
endif()

#absl
if(USE_SYSTEM_DEPS)
    find_package(absl QUIET)
endif()
if(absl_FOUND AND TARGET absl::flat_hash_map AND TARGET absl::hash AND TARGET absl::btree)
    message(STATUS "Using system absl")
else()
    check_compiled_dep(absl ${ABSL_DIR})
    add_subdirectory(${ABSL_DIR})
endif()

set(ABSL_TARGETS
    flat_hash_map
    hash
    btree
)

foreach(tgt IN LISTS ABSL_TARGETS)
    add_library(ab_${tgt} INTERFACE)
    target_link_libraries(ab_${tgt} 
        INTERFACE 
            absl::${tgt} 
    )
    target_compile_options(ab_${tgt} INTERFACE -w)
endforeach()

#GENERATED 
add_subdirectory(generated)

add_library(generated_headers INTERFACE)
target_include_directories(generated_headers
    INTERFACE
        ${GENERATED_DIR}
)

#SOURCE
add_subdirectory(src)

#TESTS
add_subdirectory(test)










