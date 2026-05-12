set(CLASSIFY_SCALAR_TEST_CXX_STANDARD 17 CACHE STRING "C++ standard for classify_scalar Catch2 tests")
option(CLASSIFY_SCALAR_BUILD_CATCH2_TESTS "Build Catch2 behavior tests" ON)
option(CLASSIFY_SCALAR_ENABLE_COVERAGE "Build classify_scalar tests with coverage instrumentation" OFF)

function(classify_scalar_enable_test_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE
            /W4
            /permissive-
            /EHsc)
    else()
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic)
    endif()
endfunction()

function(classify_scalar_enable_test_coverage target_name)
    if(CLASSIFY_SCALAR_ENABLE_COVERAGE AND NOT MSVC)
        target_compile_definitions(${target_name} PRIVATE CLASSIFY_SCALAR_DISABLE_FORCE_INLINE)
        target_compile_options(${target_name} PRIVATE --coverage -O0 -g)
        target_link_options(${target_name} PRIVATE --coverage)
    endif()
endfunction()

if(CLASSIFY_SCALAR_BUILD_CATCH2_TESTS)
    find_package(Catch2 3 CONFIG QUIET)

    if(NOT Catch2_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.6.0)
        FetchContent_MakeAvailable(Catch2)
    endif()

    add_executable(classify_scalar_tests
        test_bool.cpp
        test_classify_scalar.cpp
        test_decimal_int.cpp
        test_float.cpp
        test_float_helpers.cpp
        test_hex.cpp
        test_macro_hygiene.cpp
        test_null_string_trim.cpp
        test_numeric_only.cpp
        test_outputs.cpp
        test_parse_scalar.cpp
        test_policy_pack.cpp
        test_telephone_policy.cpp
        test_timestamp.cpp)

    target_link_libraries(classify_scalar_tests
        PRIVATE
            classify_scalar::classify_scalar
            Catch2::Catch2WithMain)

    set_property(TARGET classify_scalar_tests PROPERTY CXX_STANDARD ${CLASSIFY_SCALAR_TEST_CXX_STANDARD})
    set_property(TARGET classify_scalar_tests PROPERTY CXX_STANDARD_REQUIRED ON)
    set_property(TARGET classify_scalar_tests PROPERTY CXX_EXTENSIONS OFF)

    classify_scalar_enable_test_warnings(classify_scalar_tests)
    classify_scalar_enable_test_coverage(classify_scalar_tests)

    add_test(NAME classify_scalar_tests COMMAND classify_scalar_tests)

    add_executable(classify_scalar_float_fallback_tests
        test_float_fallback.cpp)

    target_link_libraries(classify_scalar_float_fallback_tests
        PRIVATE
            classify_scalar::classify_scalar
            Catch2::Catch2WithMain)

    set_property(TARGET classify_scalar_float_fallback_tests PROPERTY CXX_STANDARD ${CLASSIFY_SCALAR_TEST_CXX_STANDARD})
    set_property(TARGET classify_scalar_float_fallback_tests PROPERTY CXX_STANDARD_REQUIRED ON)
    set_property(TARGET classify_scalar_float_fallback_tests PROPERTY CXX_EXTENSIONS OFF)

    classify_scalar_enable_test_warnings(classify_scalar_float_fallback_tests)
    classify_scalar_enable_test_coverage(classify_scalar_float_fallback_tests)

    add_test(NAME classify_scalar_float_fallback_tests COMMAND classify_scalar_float_fallback_tests)
endif()

add_executable(classify_scalar_cxx11_smoke
    test_cxx11_smoke.cpp)

target_link_libraries(classify_scalar_cxx11_smoke
    PRIVATE
        classify_scalar::classify_scalar)

set_property(TARGET classify_scalar_cxx11_smoke PROPERTY CXX_STANDARD 11)
set_property(TARGET classify_scalar_cxx11_smoke PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET classify_scalar_cxx11_smoke PROPERTY CXX_EXTENSIONS OFF)

classify_scalar_enable_test_warnings(classify_scalar_cxx11_smoke)
classify_scalar_enable_test_coverage(classify_scalar_cxx11_smoke)

add_test(NAME classify_scalar_cxx11_smoke COMMAND classify_scalar_cxx11_smoke)

add_executable(classify_scalar_cxx20_concepts
    test_cxx20_concepts.cpp)

target_link_libraries(classify_scalar_cxx20_concepts
    PRIVATE
        classify_scalar::classify_scalar)

set_property(TARGET classify_scalar_cxx20_concepts PROPERTY CXX_STANDARD 20)
set_property(TARGET classify_scalar_cxx20_concepts PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET classify_scalar_cxx20_concepts PROPERTY CXX_EXTENSIONS OFF)

classify_scalar_enable_test_warnings(classify_scalar_cxx20_concepts)
classify_scalar_enable_test_coverage(classify_scalar_cxx20_concepts)

add_test(NAME classify_scalar_cxx20_concepts COMMAND classify_scalar_cxx20_concepts)
