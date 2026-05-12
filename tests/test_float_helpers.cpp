#include <catch2/catch_test_macros.hpp>

#include <classify_scalar.hpp>

#include <string>

using classify_scalar::scalar_bigfloat;
using classify_scalar::scalar_string;

TEST_CASE("floating pow10 helper handles exponents beyond the lookup table") {
    const long double value = classify_scalar::detail::floating::pow10_integer(20);

    CHECK(value / 1000000000000000000.0L == 100.0L);
    CHECK(classify_scalar::detail::floating::pow10_integer(-20) == 1.0L / value);
}

TEST_CASE("floating helpers classify huge syntax as bigfloat") {
    typedef classify_scalar::policy_pack<
        classify_scalar::builtin_numeric_policy<','>,
        classify_scalar::builtin_bool_policy> comma_decimal_pack;

    std::string oversized_dot = "1.";
    oversized_dot.append(4096, '0');
    CHECK(classify_scalar::classify_scalar(oversized_dot.c_str(), oversized_dot.c_str() + oversized_dot.size()) == scalar_bigfloat);

    std::string oversized_comma = "1,";
    oversized_comma.append(4096, '0');
    CHECK(classify_scalar::classify_scalar(
        oversized_comma.c_str(),
        oversized_comma.c_str() + oversized_comma.size(),
        classify_scalar::classify_only_output(),
        comma_decimal_pack()) == scalar_bigfloat);

    CHECK(classify_scalar::classify_scalar("1e309") == scalar_bigfloat);
    CHECK(classify_scalar::classify_scalar(
        "1e309",
        classify_scalar::classify_only_output(),
        comma_decimal_pack()) == scalar_bigfloat);
}

TEST_CASE("floating separator normalization rejects malformed comma decimals") {
    typedef classify_scalar::policy_pack<
        classify_scalar::builtin_numeric_policy<','>,
        classify_scalar::builtin_bool_policy> comma_decimal_pack;

    CHECK(classify_scalar::classify_scalar(
        "1,2 e3",
        classify_scalar::classify_only_output(),
        comma_decimal_pack()) == scalar_string);

    CHECK(classify_scalar::classify_scalar(
        "1,2abc",
        classify_scalar::classify_only_output(),
        comma_decimal_pack()) == scalar_string);
}
