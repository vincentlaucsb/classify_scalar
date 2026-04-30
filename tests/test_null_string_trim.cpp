#include <catch2/catch_test_macros.hpp>

#include <classify_scalar.hpp>

#include <cstdint>

using classify_scalar::scalar_int;
using classify_scalar::scalar_null;
using classify_scalar::scalar_string;

TEST_CASE("classifies null and string spans") {
    CHECK(classify_scalar::classify_scalar("   \t\r\n") == scalar_null);
    CHECK(classify_scalar::classify_scalar("hello") == scalar_string);
    CHECK(classify_scalar::classify_scalar("123 main") == scalar_string);
    CHECK(classify_scalar::classify_scalar("1 2") == scalar_string);
}

TEST_CASE("trims ASCII whitespace by default") {
    std::int64_t integer = 0;
    long double number = 0;
    bool boolean = false;

    CHECK(classify_scalar::classify_scalar("   42  ", classify_scalar::output_refs(number, integer, boolean)) == scalar_int);
    CHECK(integer == 42);
}

TEST_CASE("can preserve exact scalar boundaries") {
    CHECK(classify_scalar::classify_scalar<classify_scalar::ScalarKind, false>("   42  ") == scalar_string);
    CHECK(classify_scalar::classify_scalar<classify_scalar::ScalarKind, false>("42") == scalar_int);
}
