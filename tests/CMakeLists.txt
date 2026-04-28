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
        test_classify_scalar.cpp)

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
