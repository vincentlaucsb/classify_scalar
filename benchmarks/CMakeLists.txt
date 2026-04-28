cmake_minimum_required(VERSION 3.20)

project(classify_scalar_benchmarks
    VERSION 0.1.0
    DESCRIPTION "classify_scalar benchmark suite"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(benchmark CONFIG QUIET)

if(NOT benchmark_FOUND)
    include(FetchContent)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.5)
    FetchContent_MakeAvailable(benchmark)
endif()

add_executable(classify_scalar_benchmarks
    classify_scalar_benchmarks.cpp)

target_include_directories(classify_scalar_benchmarks
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../include")

target_link_libraries(classify_scalar_benchmarks
    PRIVATE
        benchmark::benchmark_main)

if(MSVC)
    target_compile_options(classify_scalar_benchmarks PRIVATE /W4 /permissive-)
else()
    target_compile_options(classify_scalar_benchmarks PRIVATE -Wall -Wextra -Wpedantic)
endif()
