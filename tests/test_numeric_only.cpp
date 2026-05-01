#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <classify_scalar.hpp>

#include <cstdint>

using classify_scalar::scalar_float;
using classify_scalar::scalar_int;
using classify_scalar::scalar_null;
using classify_scalar::scalar_string;

TEST_CASE("numeric-only classification excludes bool and timestamp policies") {
    CHECK(classify_scalar::classify_scalar(
        "true",
        classify_scalar::classify_only_output(),
        classify_scalar::numeric_policy_pack()) == scalar_string);
    CHECK(classify_scalar::classify_scalar(
        "2024-01-31",
        classify_scalar::classify_only_output(),
        classify_scalar::numeric_policy_pack()) == scalar_string);
    CHECK(classify_scalar::classify_scalar(
        "42",
        classify_scalar::classify_only_output(),
        classify_scalar::numeric_policy_pack()) == scalar_int);
    CHECK(classify_scalar::classify_scalar(
        "3.14",
        classify_scalar::classify_only_output(),
        classify_scalar::numeric_policy_pack()) == scalar_float);
    CHECK(classify_scalar::classify_scalar(
        "   ",
        classify_scalar::classify_only_output(),
        classify_scalar::numeric_policy_pack()) == scalar_null);
}

TEST_CASE("numeric policies select decimal symbols at compile time") {
    typedef classify_scalar::policy_pack<
        classify_scalar::builtin_numeric_policy<','> > comma_numeric_policy_pack;

    std::int64_t integer = 0;
    long double number = 0;
    bool boolean = false;

    CHECK(classify_scalar::classify_scalar(
        "3.14",
        classify_scalar::output_refs(number, integer, boolean),
        classify_scalar::numeric_policy_pack()) == scalar_float);
    CHECK(static_cast<double>(number) == Catch::Approx(3.14));

    CHECK(classify_scalar::classify_scalar(
        "3,14",
        classify_scalar::output_refs(number, integer, boolean),
        comma_numeric_policy_pack()) == scalar_float);
    CHECK(static_cast<double>(number) == Catch::Approx(3.14));

    CHECK(classify_scalar::classify_scalar(
        "3,14",
        classify_scalar::classify_only_output(),
        classify_scalar::numeric_policy_pack()) == scalar_string);
    CHECK(classify_scalar::classify_scalar(
        "3.14",
        classify_scalar::classify_only_output(),
        comma_numeric_policy_pack()) == scalar_string);
}
