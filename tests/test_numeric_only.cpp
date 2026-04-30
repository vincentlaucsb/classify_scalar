#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <classify_scalar.hpp>

#include <cstdint>

using classify_scalar::scalar_float;
using classify_scalar::scalar_int;
using classify_scalar::scalar_invalid;
using classify_scalar::scalar_null;
using classify_scalar::scalar_string;

TEST_CASE("numeric-only classification excludes bool and timestamp policies") {
    CHECK(classify_scalar::classify_numeric_scalar("true") == scalar_string);
    CHECK(classify_scalar::classify_numeric_scalar("2024-01-31") == scalar_string);
    CHECK(classify_scalar::classify_numeric_scalar("42") == scalar_int);
    CHECK(classify_scalar::classify_numeric_scalar("3.14") == scalar_float);
    CHECK(classify_scalar::classify_numeric_scalar("   ") == scalar_null);
}

TEST_CASE("runtime decimal symbol helper dispatches common numeric policies") {
    std::int64_t integer = 0;
    long double number = 0;
    bool boolean = false;

    CHECK(classify_scalar::classify_numeric_scalar_with_decimal_symbol(
        "3.14",
        '.',
        classify_scalar::output_refs(number, integer, boolean)) == scalar_float);
    CHECK(static_cast<double>(number) == Catch::Approx(3.14));

    CHECK(classify_scalar::classify_numeric_scalar_with_decimal_symbol(
        "3,14",
        ',',
        classify_scalar::output_refs(number, integer, boolean)) == scalar_float);
    CHECK(static_cast<double>(number) == Catch::Approx(3.14));

    CHECK(classify_scalar::classify_numeric_scalar_with_decimal_symbol("3,14", '.') == scalar_string);
    CHECK(classify_scalar::classify_numeric_scalar_with_decimal_symbol("3.14", ',') == scalar_string);
    CHECK(classify_scalar::classify_numeric_scalar_with_decimal_symbol("3;14", ';') == scalar_invalid);
}
