cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0069 NEW)
project(TradingEngine LANGUAGES CXX)

#GLOBAL SETTINGS
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(USE_SYSTEM_DEPS "Use system dependencies if available" ON)

add_compile_definitions(PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

# --- PROJECT OPTIONS INTERFACE ---
add_library(project_options INTERFACE)

target_compile_options(project_options INTERFACE
    -Wall -Wextra -Wno-pedantic 
    $<$<CONFIG:Debug>:-O0 -g>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
    $<$<CONFIG:RelWithDebInfo>:-O3 -g -DNDEBUG>
    -march=native
    -mtune=native
    -mavx2
    -mavx
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<AND:$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,13>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,14>>>:-Wno-array-bounds;-Wno-stringop-truncation;-Wno-stringop-overflow>
)

# --- SANITIZER OPTIONS ---
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if(ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR "ASan and TSan cannot be enabled at the same time. Pick one!")
endif()

# --- SANITIZER LOGIC (Integrated into project_options) ---
if(ENABLE_ASAN)
    target_compile_options(project_options INTERFACE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(project_options INTERFACE -fsanitize=address)
elseif(ENABLE_TSAN)
    target_compile_options(project_options INTERFACE -fsanitize=thread -fno-omit-frame-pointer)
    target_link_options(project_options INTERFACE -fsanitize=thread)
else()
    target_compile_options(project_options INTERFACE $<$<NOT:$<CONFIG:Release>>:-fno-omit-frame-pointer>)
endif()

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
if(clickhouse-cpp_FOUND AND TARGET clickhouse-cpp-lib)
    message(STATUS "Using system clickhouse-cpp")
else()
    check_compiled_dep(clickhouse-cpp-lib ${CLICKHOUSE_DIR})

    set(CMAKE_CXX_FLAGS_BACKUP "${CMAKE_CXX_FLAGS}")
    add_subdirectory(${CLICKHOUSE_DIR})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_BACKUP}")
    
    target_compile_options(clickhouse-cpp-lib PRIVATE -w)
endif()

if(ENABLE_ASAN)
    target_compile_options(clickhouse-cpp-lib PRIVATE -fsanitize=address -fno-omit-frame-pointer)
endif()

#absl 
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

if(ENABLE_ASAN)
    foreach(tgt IN LISTS ABSL_TARGETS)
        target_compile_options(ab_${tgt} INTERFACE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(ab_${tgt} INTERFACE -fsanitize=address)
    endforeach()
elseif(ENABLE_TSAN)
    foreach(tgt IN LISTS ABSL_TARGETS)
        target_compile_options(ab_${tgt} INTERFACE -fsanitize=thread -fno-omit-frame-pointer)
        target_link_options(ab_${tgt} INTERFACE -fsanitize=thread)
    endforeach()
endif()

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










