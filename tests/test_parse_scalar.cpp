#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <classify_scalar.hpp>

#include <cstdint>
#include <type_traits>

static_assert(std::is_same<
    classify_scalar::detail::scalar_home<classify_scalar::scalar_bool>::type,
    bool>::value, "scalar_bool parses to bool");
static_assert(std::is_same<
    classify_scalar::detail::scalar_home<classify_scalar::scalar_int8>::type,
    std::int8_t>::value, "scalar_int8 parses to int8");
static_assert(std::is_same<
    classify_scalar::detail::scalar_home<classify_scalar::scalar_int64>::type,
    std::int64_t>::value, "scalar_int64 parses to int64");
static_assert(std::is_same<
    classify_scalar::detail::scalar_home<classify_scalar::scalar_float>::type,
    double>::value, "scalar_float parses to double");
static_assert(std::is_same<
    classify_scalar::detail::scalar_home<classify_scalar::scalar_timestamp>::type,
    std::uint64_t>::value, "scalar_timestamp parses to UTC unix milliseconds");

template<classify_scalar::ScalarKind Kind, std::size_t Size, typename Output>
bool parse_literal(const char (&value)[Size], Output& out) {
    return classify_scalar::parse_scalar<Kind>(value, value + Size - 1, out);
}

template<std::size_t Size>
bool parse_float_literal(const char (&value)[Size], double& out, const char decimal_symbol = '.') {
    return classify_scalar::parse_float(value, value + Size - 1, out, decimal_symbol);
}

TEST_CASE("explicit hex parsing accepts bare hexadecimal") {
    std::int64_t value = 0;

    CHECK(classify_scalar::classify_scalar("DEADBEEF") == classify_scalar::scalar_string);
    CHECK_FALSE(parse_literal<classify_scalar::scalar_int64>("DEADBEEF", value));
    CHECK(classify_scalar::parse_hex("DEADBEEF", value));
    CHECK(value == 0xDEADBEEFULL);

    CHECK(parse_literal<classify_scalar::scalar_int64>("42", value));
    CHECK(value == 42);

    CHECK(parse_literal<classify_scalar::scalar_int64>("0x10", value));
    CHECK(value == 16);

    CHECK(classify_scalar::parse_hex("-FF", value));
    CHECK(value == -255);

    CHECK_FALSE(classify_scalar::parse_hex("0xgg", value));
}

TEST_CASE("explicit scalar parsers bypass classifier policy order") {
    bool boolean = false;
    std::int64_t integer = 0;
    std::int8_t tiny_integer = 0;
    std::int32_t medium_integer = 0;
    std::uint64_t timestamp = 0;
    double floating = 0;

    CHECK(parse_literal<classify_scalar::scalar_bool>("TRUE", boolean));
    CHECK(boolean);
    CHECK(parse_literal<classify_scalar::scalar_bool>("false", boolean));
    CHECK_FALSE(boolean);

    CHECK(parse_literal<classify_scalar::scalar_int64>("-42", integer));
    CHECK(integer == -42);
    CHECK_FALSE(parse_literal<classify_scalar::scalar_int64>("9223372036854775808", integer));

    const char int8_max[] = "127";
    const char int8_overflow[] = "128";
    const char int32_max[] = "2147483647";
    const char int32_overflow[] = "2147483648";

    CHECK(classify_scalar::parse_scalar<std::int8_t>(int8_max, int8_max + 3, tiny_integer));
    CHECK(tiny_integer == 127);
    CHECK_FALSE(classify_scalar::parse_scalar<std::int8_t>(int8_overflow, int8_overflow + 3, tiny_integer));
    CHECK(classify_scalar::parse_scalar<std::int32_t>(int32_max, int32_max + 10, medium_integer));
    CHECK(medium_integer == 2147483647);
    CHECK_FALSE(classify_scalar::parse_scalar<std::int32_t>(int32_overflow, int32_overflow + 10, medium_integer));

    CHECK(parse_literal<classify_scalar::scalar_timestamp>("2024-01-31", timestamp));
    CHECK(timestamp == 1706659200000ULL);
    CHECK(parse_literal<classify_scalar::scalar_timestamp>("2024-01-31T23:59:58.123Z", timestamp));
    CHECK(timestamp == 1706745598123ULL);
    CHECK(parse_literal<classify_scalar::scalar_timestamp>("2024-01-31T23:59:58+07:30", timestamp));
    CHECK(timestamp == 1706718598000ULL);
    CHECK(parse_literal<classify_scalar::scalar_timestamp>("2024-01-31T23:59:58+0730", timestamp));
    CHECK(timestamp == 1706718598000ULL);
    CHECK(parse_literal<classify_scalar::scalar_timestamp>("2024-01-31T23:59:58-05:00", timestamp));
    CHECK(timestamp == 1706763598000ULL);
    CHECK(parse_literal<classify_scalar::scalar_timestamp>("2021-04-05T10:14:57-0600", timestamp));
    CHECK(timestamp == 1617639297000ULL);
    CHECK_FALSE(parse_literal<classify_scalar::scalar_timestamp>("1969-12-31T23:59:59Z", timestamp));
    CHECK_FALSE(parse_literal<classify_scalar::scalar_timestamp>("2024-13-31", timestamp));

    CHECK(parse_literal<classify_scalar::scalar_float>("1e-3", floating));
    CHECK(floating == Catch::Approx(0.001));
}

TEST_CASE("explicit float parser accepts runtime decimal symbols") {
    double floating = 0;

    CHECK(parse_float_literal("3.14", floating, '.'));
    CHECK(floating == Catch::Approx(3.14));

    CHECK(parse_float_literal("3,14", floating, ','));
    CHECK(floating == Catch::Approx(3.14));

    CHECK(parse_float_literal("-1,25e2", floating, ','));
    CHECK(floating == Catch::Approx(-125.0));

    CHECK(parse_float_literal("42", floating));
    CHECK(floating == Catch::Approx(42.0));

    CHECK_FALSE(parse_float_literal("3,14", floating, '.'));
    CHECK_FALSE(parse_float_literal("3.14", floating, ','));
    CHECK_FALSE(parse_float_literal("3;14", floating, ';'));
}
