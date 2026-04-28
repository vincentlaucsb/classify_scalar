#include <catch2/catch_test_macros.hpp>

#include <classify_scalar/classify_scalar.hpp>

#include <cstdint>

using classify_scalar::scalar_bool;
using classify_scalar::scalar_string;

TEST_CASE("classifies booleans case-insensitively") {
    bool value = false;
    long double number = 0;
    std::int64_t integer = 0;

    CHECK(classify_scalar::classify_scalar("true", classify_scalar::output_refs(number, integer, value)) == scalar_bool);
    CHECK(value);

    CHECK(classify_scalar::classify_scalar("FALSE", classify_scalar::output_refs(number, integer, value)) == scalar_bool);
    CHECK_FALSE(value);
}

TEST_CASE("boolean recognition can be disabled at compile time") {
    CHECK(classify_scalar::classify_scalar<true, false>("true") == scalar_string);
}
