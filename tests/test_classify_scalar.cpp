#include <catch2/catch_test_macros.hpp>

#include <classify_scalar/classify_scalar.hpp>

#include <cstdint>
#include <limits>
#include <string_view>

using classify_scalar::scalar_bool;
using classify_scalar::scalar_float;
using classify_scalar::scalar_invalid;
using classify_scalar::scalar_int;
using classify_scalar::scalar_null;
using classify_scalar::scalar_outputs;
using classify_scalar::scalar_string;

TEST_CASE("classifies null and strings") {
    CHECK(classify_scalar::classify_scalar("") == scalar_null);
    CHECK(classify_scalar::classify_scalar(std::string_view{}) == scalar_null);
    CHECK(classify_scalar::classify_scalar("   \t\r\n") == scalar_null);
    CHECK(classify_scalar::classify_scalar("hello") == scalar_string);
    CHECK(classify_scalar::classify_scalar("123 main") == scalar_string);
    CHECK(classify_scalar::classify_scalar("1 2") == scalar_string);
}

TEST_CASE("trims ASCII whitespace by default") {
    std::int64_t integer = 0;
    long double number = 0;
    bool boolean = false;

    CHECK(classify_scalar::classify_scalar("   42  ", scalar_outputs{&number, &integer, &boolean}) == scalar_int);
    CHECK(integer == 42);
}

TEST_CASE("can preserve exact scalar boundaries") {
    CHECK(classify_scalar::classify_scalar<false>("   42  ") == scalar_string);
    CHECK(classify_scalar::classify_scalar<false>("42") == scalar_int);
}

TEST_CASE("classifies booleans case-insensitively") {
    bool value = false;

    long double number = 0;
    std::int64_t integer = 0;

    CHECK(classify_scalar::classify_scalar("true", scalar_outputs{&number, &integer, &value}) == scalar_bool);
    CHECK(value);

    CHECK(classify_scalar::classify_scalar("FALSE", scalar_outputs{&number, &integer, &value}) == scalar_bool);
    CHECK_FALSE(value);

    CHECK(classify_scalar::classify_scalar<true, false>("true") == scalar_string);
}

TEST_CASE("classifies decimal integers and preserves signed minimum") {
    std::int64_t integer = 0;
    long double number = 0;
    bool boolean = false;

    CHECK(classify_scalar::classify_scalar("-2147483648", scalar_outputs{&number, &integer, &boolean}) == scalar_int);
    CHECK(integer == -2147483648LL);
    CHECK(number == -2147483648.0L);

    CHECK(classify_scalar::classify_scalar("+42", scalar_outputs{&number, &integer, &boolean}) == scalar_int);
    CHECK(integer == 42);

    CHECK(classify_scalar::classify_scalar("-9223372036854775808", scalar_outputs{&number, &integer, &boolean}) == scalar_int);
    CHECK(integer == std::numeric_limits<std::int64_t>::min());

    CHECK(classify_scalar::classify_scalar("9223372036854775808") == scalar_string);
}

TEST_CASE("classifies hexadecimal integers") {
    std::int64_t integer = 0;
    long double number = 0;
    bool boolean = false;

    CHECK(classify_scalar::classify_scalar("0x10", scalar_outputs{&number, &integer, &boolean}) == scalar_int);
    CHECK(integer == 16);

    CHECK(classify_scalar::classify_scalar("0x1e", scalar_outputs{&number, &integer, &boolean}) == scalar_int);
    CHECK(integer == 30);

    CHECK(classify_scalar::classify_scalar("0xff", scalar_outputs{&number, &integer, &boolean}) == scalar_int);
    CHECK(integer == 255);

    CHECK(classify_scalar::classify_scalar("-0X80000000", scalar_outputs{&number, &integer, &boolean}) == scalar_int);
    CHECK(integer == -2147483648LL);

    CHECK(classify_scalar::classify_scalar("0x") == scalar_string);
    CHECK(classify_scalar::classify_scalar("0xgg") == scalar_string);
    CHECK(classify_scalar::classify_scalar<true, true, false>("0x10") == scalar_string);
}

TEST_CASE("classifies floats and exponential notation") {
    std::int64_t integer = 0;
    long double number = 0;
    bool boolean = false;

    CHECK(classify_scalar::classify_scalar("3.5", scalar_outputs{&number, &integer, &boolean}) == scalar_float);
    CHECK(number == 3.5L);

    CHECK(classify_scalar::classify_scalar("+3.5", scalar_outputs{&number, &integer, &boolean}) == scalar_float);
    CHECK(number == 3.5L);

    CHECK(classify_scalar::classify_scalar("-1.25e2", scalar_outputs{&number, &integer, &boolean}) == scalar_int);
    CHECK(integer == -125);
    CHECK(number == -125.0L);

    CHECK(classify_scalar::classify_scalar("1e-3", scalar_outputs{&number, &integer, &boolean}) == scalar_float);
    CHECK(number == 0.001L);

    CHECK(classify_scalar::classify_scalar("1e") == scalar_string);
}

TEST_CASE("partial output requests are invalid") {
    std::int64_t integer = 0;
    CHECK(classify_scalar::classify_scalar("42", scalar_outputs{nullptr, &integer, nullptr}) == scalar_invalid);
}
