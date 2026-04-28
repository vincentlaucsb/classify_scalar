#include <catch2/catch_test_macros.hpp>

#include <classify_scalar/classify_scalar.hpp>

#include <cstdint>

using classify_scalar::scalar_bool;
using classify_scalar::scalar_int;

TEST_CASE("output_refs stores built-in scalar values") {
    std::int64_t integer = 0;
    long double number = 0;
    bool boolean = false;
    classify_scalar::builtin_output_refs outputs = classify_scalar::output_refs(number, integer, boolean);

    CHECK(classify_scalar::classify_scalar("42", outputs) == scalar_int);
    CHECK(integer == 42);
    CHECK(number == 42.0L);

    CHECK(classify_scalar::classify_scalar("false", outputs) == scalar_bool);
    CHECK_FALSE(boolean);
}
