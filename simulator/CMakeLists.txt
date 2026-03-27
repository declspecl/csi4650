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
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(_CXX_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
get_filename_component(_LLVM_ROOT "${_CXX_DIR}" DIRECTORY)
link_directories(${_LLVM_ROOT}/lib/c++)

file(
    GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/blackjack/**/*.hpp
)

add_executable(
    blackjack_simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
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

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
)

FetchContent_MakeAvailable(googletest)

file(
    GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp
)

add_executable(
    test_runner
    ${TEST_SOURCES}
    ${PROJECT_HEADERS}
)

target_include_directories(
    test_runner
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    test_runner
    PRIVATE GTest::gtest_main pthread
)
