#include <catch2/catch_test_macros.hpp>

#include <classify_scalar/classify_scalar.hpp>

#include <cstdint>
#include <limits>

using classify_scalar::scalar_int;
using classify_scalar::scalar_string;

TEST_CASE("classifies decimal integers and preserves signed minimum") {
    std::int64_t integer = 0;
    long double number = 0;
    bool boolean = false;

    CHECK(classify_scalar::classify_scalar("-2147483648", classify_scalar::output_refs(number, integer, boolean)) == scalar_int);
    CHECK(integer == -2147483648LL);
    CHECK(number == -2147483648.0L);

    CHECK(classify_scalar::classify_scalar("+42", classify_scalar::output_refs(number, integer, boolean)) == scalar_int);
    CHECK(integer == 42);

    CHECK(classify_scalar::classify_scalar("-9223372036854775808", classify_scalar::output_refs(number, integer, boolean)) == scalar_int);
    CHECK(integer == std::numeric_limits<std::int64_t>::min());
}

TEST_CASE("decimal integer overflow falls back to string") {
    CHECK(classify_scalar::classify_scalar("9223372036854775808") == scalar_string);
}
