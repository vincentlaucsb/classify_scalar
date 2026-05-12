#include <catch2/catch_test_macros.hpp>

#include <classify_scalar.hpp>

using classify_scalar::scalar_int8;
using classify_scalar::scalar_int32;
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
    CHECK(classify_scalar::classify_scalar("2021-04-05T10:14:57-0600") == scalar_timestamp);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:58+0730") == scalar_timestamp);
    CHECK(classify_scalar::classify_scalar("2024-01-31t23:59:58z") == scalar_timestamp);
}

TEST_CASE("malformed ISO timestamps fall back to string") {
    CHECK(classify_scalar::classify_scalar("202A-01-31") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-13-01") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-32") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31 23:59") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31T2") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31Tab:59") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31T24:00") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:60") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:60") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:58.") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:58q") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:58Ztail") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:58+0x:30") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:58+24:00") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:58+2400") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:58+0760") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:58+07") == scalar_string);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:58+07:3") == scalar_string);
}

TEST_CASE("timestamps accept short fractional milliseconds") {
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:58.1") == scalar_timestamp);
    CHECK(classify_scalar::classify_scalar("2024-01-31T23:59:58.12") == scalar_timestamp);
}

TEST_CASE("timestamp parser can classify without storing output") {
    const char date_time[] = "2024-01-31T23:59:58Z";
    CHECK(classify_scalar::detail::parsing_timestamp::parse_iso_timestamp(
        date_time,
        date_time + sizeof(date_time) - 1));
}

TEST_CASE("timestamp policy falls through to numeric parsing") {
    CHECK(classify_scalar::classify_scalar("20240131") == scalar_int32);
    CHECK(classify_scalar::classify_scalar("42") == scalar_int8);
}

TEST_CASE("timestamp recognition can be disabled") {
    typedef classify_scalar::numeric_bool_policy_pack no_timestamp_pack;

    CHECK(classify_scalar::classify_scalar("2024-01-31", classify_scalar::classify_only_output(), no_timestamp_pack()) == scalar_string);
}
