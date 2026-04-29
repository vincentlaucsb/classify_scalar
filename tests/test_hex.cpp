#include <catch2/catch_test_macros.hpp>

#include <classify_scalar/classify_scalar.hpp>

#include <cstdint>

using classify_scalar::scalar_int;
using classify_scalar::scalar_string;

TEST_CASE("classifies hexadecimal integers") {
    std::int64_t integer = 0;
    long double number = 0;
    bool boolean = false;

    CHECK(classify_scalar::classify_scalar("0x10", classify_scalar::output_refs(number, integer, boolean)) == scalar_int);
    CHECK(integer == 16);

    CHECK(classify_scalar::classify_scalar("0x1e", classify_scalar::output_refs(number, integer, boolean)) == scalar_int);
    CHECK(integer == 30);

    CHECK(classify_scalar::classify_scalar("0xff", classify_scalar::output_refs(number, integer, boolean)) == scalar_int);
    CHECK(integer == 255);

    CHECK(classify_scalar::classify_scalar("-0X80000000", classify_scalar::output_refs(number, integer, boolean)) == scalar_int);
    CHECK(integer == -2147483648LL);
}

TEST_CASE("malformed hexadecimal values fall back to string") {
    CHECK(classify_scalar::classify_scalar("0x") == scalar_string);
    CHECK(classify_scalar::classify_scalar("0xgg") == scalar_string);
}

TEST_CASE("hexadecimal recognition can be disabled at compile time") {
    typedef classify_scalar::policy_pack<
        classify_scalar::builtin_timestamp_policy,
        classify_scalar::builtin_numeric_policy<false>,
        classify_scalar::builtin_bool_policy> no_hex_pack;

    CHECK(classify_scalar::classify_scalar("0x10", classify_scalar::classify_only_output(), no_hex_pack()) == scalar_string);
}
