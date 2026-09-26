cmake_minimum_required(VERSION 3.20)

project(classify_scalar
    VERSION 1.1.2
    DESCRIPTION "Header-only scalar classification for C++"
    LANGUAGES CXX)

add_library(classify_scalar INTERFACE)
add_library(classify_scalar::classify_scalar ALIAS classify_scalar)

target_include_directories(classify_scalar
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_compile_features(classify_scalar INTERFACE cxx_std_11)

# Probe in C++17 even when consumers use C++11; the header also guards the
# language version. Keep these settings local to the probe.
function(classify_scalar_check_float_from_chars)
    include(CheckCXXSourceCompiles)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    check_cxx_source_compiles("
        #include <charconv>
        int main() {
            double value;
            const char* text = \"3.14\";
            return std::from_chars(text, text + 4, value).ec != std::errc();
        }" CLASSIFY_SCALAR_CMAKE_FLOAT_FROM_CHARS)
    target_compile_definitions(classify_scalar INTERFACE
        CLASSIFY_SCALAR_USE_STD_FLOAT_FROM_CHARS=$<BOOL:${CLASSIFY_SCALAR_CMAKE_FLOAT_FROM_CHARS}>)
endfunction()
classify_scalar_check_float_from_chars()

option(CLASSIFY_SCALAR_BUILD_TESTS "Build classify_scalar tests" ON)

if(CLASSIFY_SCALAR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
