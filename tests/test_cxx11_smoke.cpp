#include <classify_scalar.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

namespace {

int failures = 0;
volatile int runtime_guard = 0;

void check_failed(const char* file, int line, const char* expression) {
    ++failures;
    std::cerr << file << ':' << line << ": CHECK(" << expression << ") failed\n";
}

#define CHECK(expression) \
    do { \
        if (!(expression)) \
            check_failed(__FILE__, __LINE__, #expression); \
    } while (false)

std::string make_runtime_string(const char* value) {
    std::string out;
    for (const char* current = value; *current; ++current)
        out.push_back(*current);
    if (runtime_guard)
        out.push_back('!');
    return out;
}

const char* first(const std::string& value) {
    return value.data();
}

const char* last(const std::string& value) {
    return value.data() + value.size();
}

bool near_value(long double lhs, long double rhs) {
    return std::fabs(static_cast<double>(lhs - rhs)) < 0.000000001;
}

classify_scalar::ScalarKind classify_runtime(const char* value) {
    const std::string input = make_runtime_string(value);
    return classify_scalar::classify_scalar(first(input), last(input));
}

template<typename Output>
classify_scalar::ScalarKind classify_runtime(
    const char* value,
    Output output) {
    const std::string input = make_runtime_string(value);
    return classify_scalar::classify_scalar(first(input), last(input), output);
}

template<typename Output, typename Policy>
classify_scalar::ScalarKind classify_runtime(
    const char* value,
    Output output,
    Policy policy) {
    const std::string input = make_runtime_string(value);
    return classify_scalar::classify_scalar(first(input), last(input), output, policy);
}

void test_null_string_and_trimming() {
    const char* null_pointer = 0;
    CHECK(classify_scalar::classify_scalar(null_pointer, null_pointer) == classify_scalar::scalar_string);

    const std::string empty = make_runtime_string("");
    CHECK(classify_scalar::classify_scalar(first(empty), last(empty)) == classify_scalar::scalar_null);

    CHECK(classify_runtime("   ") == classify_scalar::scalar_null);
    CHECK(classify_runtime("  42 \t") == classify_scalar::scalar_int8);

    const std::string spaced = make_runtime_string("  42 ");
    CHECK((classify_scalar::classify_scalar<classify_scalar::ScalarKind, false>(
        first(spaced),
        last(spaced)) == classify_scalar::scalar_string));
}

void test_signed_integer_classification() {
    long double number = 0;
    std::int64_t integer = 0;
    bool boolean = false;
    classify_scalar::builtin_output_refs output = classify_scalar::output_refs(number, integer, boolean);

    CHECK(classify_runtime("42", output) == classify_scalar::scalar_int8);
    CHECK(integer == 42);
    CHECK(classify_runtime("-42", output) == classify_scalar::scalar_int8);
    CHECK(integer == -42);
    CHECK(classify_runtime("+42", output) == classify_scalar::scalar_int8);
    CHECK(integer == 42);

    CHECK(classify_runtime("127") == classify_scalar::scalar_int8);
    CHECK(classify_runtime("128") == classify_scalar::scalar_int16);
    CHECK(classify_runtime("-128") == classify_scalar::scalar_int8);
    CHECK(classify_runtime("-129") == classify_scalar::scalar_int16);
    CHECK(classify_runtime("32767") == classify_scalar::scalar_int16);
    CHECK(classify_runtime("32768") == classify_scalar::scalar_int32);
    CHECK(classify_runtime("2147483647") == classify_scalar::scalar_int32);
    CHECK(classify_runtime("2147483648") == classify_scalar::scalar_int64);
    CHECK(classify_runtime("9223372036854775807") == classify_scalar::scalar_int64);
    CHECK(classify_runtime("9223372036854775808") == classify_scalar::scalar_bigint);
    CHECK(classify_runtime("-9223372036854775808") == classify_scalar::scalar_int64);
    CHECK(classify_runtime("-9223372036854775809") == classify_scalar::scalar_bigint);
    CHECK(classify_runtime("123456789012345678901234567890") == classify_scalar::scalar_bigint);
}

void test_hex_parsing() {
    std::int64_t signed_hex = 0;
    std::uint8_t unsigned_hex = 0;
    std::int16_t small_signed_hex = 0;

    std::string bare = make_runtime_string("FF");
    CHECK(classify_scalar::parse_hex(first(bare), last(bare), unsigned_hex));
    CHECK(unsigned_hex == 255);

    std::string prefixed = make_runtime_string("0xFF");
    CHECK(classify_scalar::parse_hex(first(prefixed), last(prefixed), signed_hex));
    CHECK(signed_hex == 255);

    std::string negative = make_runtime_string("-FF");
    CHECK(classify_scalar::parse_hex(first(negative), last(negative), small_signed_hex));
    CHECK(small_signed_hex == -255);

    std::string unsigned_negative = make_runtime_string("-1");
    CHECK(!classify_scalar::parse_hex(first(unsigned_negative), last(unsigned_negative), unsigned_hex));

    std::string unsigned_overflow = make_runtime_string("100");
    CHECK(!classify_scalar::parse_hex(first(unsigned_overflow), last(unsigned_overflow), unsigned_hex));
}

void test_floats_and_decimal_symbols() {
    long double number = 0;
    std::int64_t integer = 0;
    bool boolean = false;
    classify_scalar::builtin_output_refs output = classify_scalar::output_refs(number, integer, boolean);

    CHECK(classify_runtime("1e-3", output) == classify_scalar::scalar_float);
    CHECK(near_value(number, 0.001L));
    CHECK(classify_runtime("-2.5", output) == classify_scalar::scalar_float);
    CHECK(near_value(number, -2.5L));
    CHECK(classify_runtime("+3.14", output) == classify_scalar::scalar_float);
    CHECK(near_value(number, 3.14L));

    double parsed = 0;
    std::string comma = make_runtime_string("3,14");
    CHECK(classify_scalar::parse_float(first(comma), last(comma), parsed, ','));
    CHECK(near_value(parsed, 3.14L));

    std::string non_numeric = make_runtime_string("stroustrup");
    CHECK(!classify_scalar::parse_float(first(non_numeric), last(non_numeric), parsed, ','));
}

void test_bool_timestamp_and_outputs() {
    long double number = 0;
    std::int64_t integer = 0;
    std::uint64_t timestamp = 0;
    bool boolean = false;
    classify_scalar::builtin_output_refs output(number, integer, boolean, timestamp);

    CHECK(classify_runtime("true", output) == classify_scalar::scalar_bool);
    CHECK(boolean);
    CHECK(classify_runtime("FALSE", output) == classify_scalar::scalar_bool);
    CHECK(!boolean);

    CHECK(classify_runtime("2024-01-31", output) == classify_scalar::scalar_timestamp);
    CHECK(timestamp == 1706659200000ULL);
    CHECK(classify_runtime("2021-04-05T10:14:57-0600", output) == classify_scalar::scalar_timestamp);
    CHECK(timestamp == 1617639297000ULL);

    CHECK(classify_runtime("2147483648", output) == classify_scalar::scalar_int64);
    CHECK(integer == 2147483648LL);

    CHECK(classify_runtime(
        "42",
        classify_scalar::classify_only_output()) == classify_scalar::scalar_int8);
}

void test_policy_packs() {
    CHECK(classify_runtime(
        "true",
        classify_scalar::classify_only_output(),
        classify_scalar::numeric_policy_pack()) == classify_scalar::scalar_string);

    CHECK(classify_runtime(
        "2024-01-31",
        classify_scalar::classify_only_output(),
        classify_scalar::numeric_policy_pack()) == classify_scalar::scalar_string);

    typedef classify_scalar::policy_pack<
        classify_scalar::builtin_numeric_policy<','> > comma_numeric_policy_pack;

    CHECK(classify_runtime(
        "3,14",
        classify_scalar::classify_only_output(),
        comma_numeric_policy_pack()) == classify_scalar::scalar_float);

    CHECK(classify_runtime(
        "3.14",
        classify_scalar::classify_only_output(),
        comma_numeric_policy_pack()) == classify_scalar::scalar_string);
}

} // namespace

int main() {
    test_null_string_and_trimming();
    test_signed_integer_classification();
    test_hex_parsing();
    test_floats_and_decimal_symbols();
    test_bool_timestamp_and_outputs();
    test_policy_packs();

    if (failures != 0)
        std::cerr << failures << " C++11 smoke test failure(s)\n";

    return failures == 0 ? 0 : 1;
}
