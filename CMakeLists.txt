cmake_minimum_required(VERSION 3.20)

project(classify_scalar
    VERSION 0.1.0
    DESCRIPTION "Header-only scalar classification for C++"
    LANGUAGES CXX)

add_library(classify_scalar INTERFACE)
add_library(classify_scalar::classify_scalar ALIAS classify_scalar)

target_include_directories(classify_scalar
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_compile_features(classify_scalar INTERFACE cxx_std_11)

option(CLASSIFY_SCALAR_BUILD_TESTS "Build classify_scalar tests" ON)

if(CLASSIFY_SCALAR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
