#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <classify_scalar/classify_scalar.hpp>

#include <cstdint>

using classify_scalar::scalar_float;
using classify_scalar::scalar_int;
using classify_scalar::scalar_string;

TEST_CASE("classifies decimal floats") {
    std::int64_t integer = 0;
    long double number = 0;
    bool boolean = false;

    CHECK(classify_scalar::classify_scalar("3.5", classify_scalar::output_refs(number, integer, boolean)) == scalar_float);
    CHECK(static_cast<double>(number) == Catch::Approx(3.5));

    CHECK(classify_scalar::classify_scalar("+3.5", classify_scalar::output_refs(number, integer, boolean)) == scalar_float);
    CHECK(static_cast<double>(number) == Catch::Approx(3.5));

    CHECK(classify_scalar::classify_scalar(".67", classify_scalar::output_refs(number, integer, boolean)) == scalar_float);
    CHECK(static_cast<double>(number) == Catch::Approx(0.67));

    CHECK(classify_scalar::classify_scalar("+.67", classify_scalar::output_refs(number, integer, boolean)) == scalar_float);
    CHECK(static_cast<double>(number) == Catch::Approx(0.67));

    CHECK(classify_scalar::classify_scalar("-.67", classify_scalar::output_refs(number, integer, boolean)) == scalar_float);
    CHECK(static_cast<double>(number) == Catch::Approx(-0.67));
}

TEST_CASE("classifies floats with a custom decimal symbol") {
    typedef classify_scalar::policy_pack<
        classify_scalar::builtin_numeric_policy<','>,
        classify_scalar::builtin_bool_policy> comma_decimal_pack;

    std::int64_t integer = 0;
    long double number = 0;
    bool boolean = false;

    CHECK(classify_scalar::classify_scalar(
        "3,14",
        classify_scalar::output_refs(number, integer, boolean),
        comma_decimal_pack()) == scalar_float);
    CHECK(static_cast<double>(number) == Catch::Approx(3.14));

    CHECK(classify_scalar::classify_scalar(
        "-1,25e2",
        classify_scalar::output_refs(number, integer, boolean),
        comma_decimal_pack()) == scalar_int);
    CHECK(integer == -125);

    CHECK(classify_scalar::classify_scalar("3,14") == scalar_string);
    CHECK(classify_scalar::classify_scalar(
        "3.14",
        classify_scalar::classify_only_output(),
        comma_decimal_pack()) == scalar_string);
}

TEST_CASE("malformed decimal floats fall back to string") {
    CHECK(classify_scalar::classify_scalar(".") == scalar_string);
    CHECK(classify_scalar::classify_scalar("+.") == scalar_string);
    CHECK(classify_scalar::classify_scalar("-.") == scalar_string);
}

TEST_CASE("classifies exponential notation") {
    std::int64_t integer = 0;
    long double number = 0;
    bool boolean = false;

    CHECK(classify_scalar::classify_scalar("-1.25e2", classify_scalar::output_refs(number, integer, boolean)) == scalar_int);
    CHECK(integer == -125);
    CHECK(number == -125.0L);

    CHECK(classify_scalar::classify_scalar("1e-3", classify_scalar::output_refs(number, integer, boolean)) == scalar_float);
    CHECK(static_cast<double>(number) == Catch::Approx(0.001));
}

TEST_CASE("malformed exponential notation falls back to string") {
    CHECK(classify_scalar::classify_scalar("1e") == scalar_string);
}
