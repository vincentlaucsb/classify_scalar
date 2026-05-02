#include <catch2/catch_test_macros.hpp>

#include <classify_scalar.hpp>

#include <cstdint>
#include <limits>

using classify_scalar::scalar_bigint;
using classify_scalar::scalar_int8;
using classify_scalar::scalar_int32;
using classify_scalar::scalar_int64;
using classify_scalar::scalar_string;

TEST_CASE("classifies decimal integers and preserves signed minimum") {
    std::int64_t integer = 0;
    long double number = 99.0L;
    bool boolean = false;

    CHECK(classify_scalar::classify_scalar("-2147483648", classify_scalar::output_refs(number, integer, boolean)) == scalar_int32);
    CHECK(integer == -2147483648LL);
    CHECK(number == 99.0L);

    CHECK(classify_scalar::classify_scalar("+42", classify_scalar::output_refs(number, integer, boolean)) == scalar_int8);
    CHECK(integer == 42);

    CHECK(classify_scalar::classify_scalar("-9223372036854775808", classify_scalar::output_refs(number, integer, boolean)) == scalar_int64);
    CHECK(integer == std::numeric_limits<std::int64_t>::min());
}

TEST_CASE("decimal integer overflow is classified as bigint") {
    CHECK(classify_scalar::classify_scalar("9223372036854775807") == scalar_int64);
    CHECK(classify_scalar::classify_scalar("9223372036854775808") == scalar_bigint);
    CHECK(classify_scalar::classify_scalar("-9223372036854775808") == scalar_int64);
    CHECK(classify_scalar::classify_scalar("-9223372036854775809") == scalar_bigint);
    CHECK(classify_scalar::classify_scalar("1234567890123456789012345678901234567890") == scalar_bigint);
    CHECK(classify_scalar::classify_scalar("-1234567890123456789012345678901234567890") == scalar_bigint);
    CHECK(classify_scalar::classify_scalar("12345678901234567890abc") == scalar_string);
}
