cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED CMAKE_CXX_COMPILER)
    if(EXISTS "/opt/homebrew/opt/llvm/bin/clang++")
        set(CMAKE_CXX_COMPILER "/opt/homebrew/opt/llvm/bin/clang++" CACHE FILEPATH "C++ compiler" FORCE)
    else()
        find_program(_CLANGXX clang++ REQUIRED)
        set(CMAKE_CXX_COMPILER "${_CLANGXX}" CACHE FILEPATH "C++ compiler" FORCE)
    endif()
endif()

get_filename_component(_CXX_NAME "${CMAKE_CXX_COMPILER}" NAME)
if(NOT _CXX_NAME MATCHES "^clang\\+\\+$")
    message(FATAL_ERROR "This project requires clang++. Chosen: ${CMAKE_CXX_COMPILER}")
endif()

project(
    blackjack_simulator
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx
)

file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hxx
)

add_executable(
    blackjack_simulator
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
)

target_include_directories(
    blackjack_simulator
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(OpenMP REQUIRED)

target_link_libraries(
    blackjack_simulator
    PRIVATE OpenMP::OpenMP_CXX
)
