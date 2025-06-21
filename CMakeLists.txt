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

# Bu custom command'i tetikleyen hedef
add_custom_target(GeneratedFiles ALL
    DEPENDS ${CMAKE_SOURCE_DIR}/include/GeneratedErrorMap.h
    DEPENDS ${CMAKE_SOURCE_DIR}/include/GeneratedIpPort.h
)

# main.cpp ve diğer kaynak dosyaları
add_executable(MarketData ${SOURCES})

# include path ve bağımlılık
target_include_directories(MarketData PRIVATE include)
add_dependencies(MarketData GeneratedFiles)
target_link_libraries(MarketData PRIVATE fmt::fmt)