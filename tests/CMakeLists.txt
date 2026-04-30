set(CLASSIFY_SCALAR_TEST_CXX_STANDARD 17 CACHE STRING "C++ standard for classify_scalar Catch2 tests")
option(CLASSIFY_SCALAR_BUILD_CATCH2_TESTS "Build Catch2 behavior tests" ON)

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
        test_hex.cpp
        test_macro_hygiene.cpp
        test_null_string_trim.cpp
        test_numeric_only.cpp
        test_outputs.cpp
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

    if(MSVC)
        target_compile_options(classify_scalar_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(classify_scalar_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME classify_scalar_tests COMMAND classify_scalar_tests)
endif()

add_executable(classify_scalar_cxx11_smoke
    test_cxx11_smoke.cpp)

target_link_libraries(classify_scalar_cxx11_smoke
    PRIVATE
        classify_scalar::classify_scalar)

set_property(TARGET classify_scalar_cxx11_smoke PROPERTY CXX_STANDARD 11)
set_property(TARGET classify_scalar_cxx11_smoke PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET classify_scalar_cxx11_smoke PROPERTY CXX_EXTENSIONS OFF)

if(MSVC)
    target_compile_options(classify_scalar_cxx11_smoke PRIVATE /W4 /permissive-)
else()
    target_compile_options(classify_scalar_cxx11_smoke PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME classify_scalar_cxx11_smoke COMMAND classify_scalar_cxx11_smoke)

add_executable(classify_scalar_cxx20_concepts
    test_cxx20_concepts.cpp)

target_link_libraries(classify_scalar_cxx20_concepts
    PRIVATE
        classify_scalar::classify_scalar)

set_property(TARGET classify_scalar_cxx20_concepts PROPERTY CXX_STANDARD 20)
set_property(TARGET classify_scalar_cxx20_concepts PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET classify_scalar_cxx20_concepts PROPERTY CXX_EXTENSIONS OFF)

if(MSVC)
    target_compile_options(classify_scalar_cxx20_concepts PRIVATE /W4 /permissive-)
else()
    target_compile_options(classify_scalar_cxx20_concepts PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME classify_scalar_cxx20_concepts COMMAND classify_scalar_cxx20_concepts)
