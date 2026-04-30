#include <catch2/catch_test_macros.hpp>

#include <classify_scalar.hpp>

using classify_scalar::scalar_int;
using classify_scalar::scalar_string;
using classify_scalar::scalar_timestamp;

TEST_CASE("classifies ISO date timestamps") {
    CHECK(classify_scalar::classify_scalar("2024-01-31") == scalar_timestamp);
    CHECK(classify_scalar::classify_scalar("2024-02-29") == scalar_timestamp);
    CHECK(classify_scalar::classify_scalar("1900-02-29") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2000-02-29") == scalar_timestamp);
}

TEST_CASE("classifies ISO date-time timestamps") {
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59") == scalar_timestamp);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:58") == scalar_timestamp);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:58.123") == scalar_timestamp);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:58Z") == scalar_timestamp);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:58+07:30") == scalar_timestamp);
    CHECK(classify_scalar::classify_scalar("2024-01-31t23:59:58z") == scalar_timestamp);
}

TEST_CASE("malformed ISO timestamps fall back to string") {
    CHECK(classify_scalar::classify_scalar("2024-13-01") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-32") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31T24:00") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:60") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:60") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:58.") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:58+24:00") == scalar_string);
}

TEST_CASE("timestamp policy falls through to numeric parsing") {
    CHECK(classify_scalar::classify_scalar("20240131") == scalar_int);
    CHECK(classify_scalar::classify_scalar("42") == scalar_int);
}

TEST_CASE("timestamp recognition can be disabled") {
    typedef classify_scalar::numeric_bool_policy_pack no_timestamp_pack;

    CHECK(classify_scalar::classify_scalar("2024-01-31", classify_scalar::classify_only_output(), no_timestamp_pack()) == scalar_string);
}
