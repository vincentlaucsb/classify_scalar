#include <catch2/catch_test_macros.hpp>

#include <classify_scalar.hpp>

#include <cstdint>

using classify_scalar::scalar_bool;
using classify_scalar::scalar_int8;
using classify_scalar::scalar_int16;
using classify_scalar::scalar_int32;
using classify_scalar::scalar_int64;

TEST_CASE("output_refs stores built-in scalar values") {
    std::int64_t integer = 0;
    long double number = 0;
    bool boolean = false;
    classify_scalar::builtin_output_refs outputs = classify_scalar::output_refs(number, integer, boolean);

    CHECK(classify_scalar::classify_scalar("42", outputs) == scalar_int8);
    CHECK(integer == 42);
    CHECK(number == 42.0L);

    CHECK(classify_scalar::classify_scalar("false", outputs) == scalar_bool);
    CHECK_FALSE(boolean);
}

TEST_CASE("classification reports the narrowest signed integer kind") {
    std::int64_t integer = 0;
    long double number = 0;
    bool boolean = false;
    classify_scalar::builtin_output_refs outputs = classify_scalar::output_refs(number, integer, boolean);

    CHECK(classify_scalar::classify_scalar("127", outputs) == scalar_int8);

    CHECK(classify_scalar::classify_scalar("128", outputs) == scalar_int16);

    CHECK(classify_scalar::classify_scalar("32768", outputs) == scalar_int32);

    CHECK(classify_scalar::classify_scalar("2147483648", outputs) == scalar_int64);

    CHECK(classify_scalar::classify_scalar("-128", outputs) == scalar_int8);

    CHECK(classify_scalar::classify_scalar("-129", outputs) == scalar_int16);

    CHECK(classify_scalar::classify_scalar("-32769", outputs) == scalar_int32);

    CHECK(classify_scalar::classify_scalar("-2147483649", outputs) == scalar_int64);
}
