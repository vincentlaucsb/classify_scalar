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
    CHECK(classify_scalar::classify_scalar<true, true, false>("0x10") == scalar_string);
}
