#include <benchmark/benchmark.h>

#include <classify_scalar.hpp>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {

enum class BenchKind : int {
    null_value,
    string,
    boolean,
    integer,
    floating,
    timestamp
};

struct bench_case {
    std::string value;
};

const std::vector<bench_case>& mixed_cases() {
    static const std::vector<bench_case> cases = {
        {""},
        {"   "},
        {"hello"},
        {"true"},
        {"FALSE"},
        {"0"},
        {"42"},
        {"-17"},
        {"2147483647"},
        {"-2147483648"},
        {"9223372036854775807"},
        {"0x10"},
        {"0x1e"},
        {"-0X80000000"},
        {"3.14159"},
        {"-1.25e2"},
        {"1e-3"},
        {"2024-01-31T23:59:58Z"},
        {"  12345  "},
        {"510 123 4567"},
        {"0xgg"},
        {"not-a-number"},
    };
    return cases;
}

const std::vector<bench_case>& int_parse_cases() {
    static const std::vector<bench_case> cases = {
        {"0"},
        {"42"},
        {"-17"},
        {"2147483647"},
        {"-2147483648"},
        {"9223372036854775807"},
        {"-9223372036854775808"},
        {"123456789"},
    };
    return cases;
}

const std::vector<bench_case>& float_parse_cases() {
    static const std::vector<bench_case> cases = {
        {"0.0"},
        {"3.14159"},
        {"-1.25e2"},
        {"1e-3"},
        {"6.02214076e23"},
        {"-2.2250738585072014e-308"},
        {"1.7976931348623157e308"},
        {".67"},
    };
    return cases;
}

inline bool ascii_space(char c) noexcept {
    switch (c) {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
    case '\f':
    case '\v':
        return true;
    default:
        return false;
    }
}

inline std::string_view trim_ascii(std::string_view value) noexcept {
    while (!value.empty() && ascii_space(value.front())) {
        value.remove_prefix(1);
    }
    while (!value.empty() && ascii_space(value.back())) {
        value.remove_suffix(1);
    }
    return value;
}

inline char lower_ascii(char c) noexcept {
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
}

inline bool iequals(std::string_view lhs, std::string_view rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        if (lower_ascii(lhs[i]) != rhs[i]) {
            return false;
        }
    }
    return true;
}

constexpr bool ascii_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}

template<std::size_t Count>
inline bool parse_digits(const char* value, int& out) noexcept {
    int parsed = 0;
    for (std::size_t i = 0; i < Count; ++i) {
        if (!ascii_digit(value[i])) {
            return false;
        }
        parsed = (parsed * 10) + (value[i] - '0');
    }
    out = parsed;
    return true;
}

constexpr bool leap_year(int year) noexcept {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

constexpr int common_days_in_month[13] = {
    0,
    31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
};

inline int month_days(int year, int month) noexcept {
    return month == 2 && leap_year(year)
        ? 29
        : common_days_in_month[month];
}

inline bool valid_date(int year, int month, int day) noexcept {
    return month >= 1 && month <= 12 && day >= 1 && day <= month_days(year, month);
}

inline bool consume_timezone(const char*& current, const char* last) noexcept {
    if (current == last) {
        return true;
    }
    if (*current == 'z' || *current == 'Z') {
        ++current;
        return current == last;
    }
    if (*current != '+' && *current != '-') {
        return false;
    }
    if (current + 6 != last || current[3] != ':') {
        return false;
    }

    int hour = 0;
    int minute = 0;
    if (!parse_digits<2>(current + 1, hour) || !parse_digits<2>(current + 4, minute)) {
        return false;
    }

    current = last;
    return hour <= 23 && minute <= 59;
}

inline bool naive_iso_timestamp(std::string_view value) noexcept {
    if (value.size() < 10) {
        return false;
    }
    const char* first = value.data();
    const char* last = first + value.size();
    if (first[4] != '-' || first[7] != '-') {
        return false;
    }

    int year = 0;
    int month = 0;
    int day = 0;
    if (!parse_digits<4>(first, year) || !parse_digits<2>(first + 5, month) || !parse_digits<2>(first + 8, day)) {
        return false;
    }
    if (!valid_date(year, month, day)) {
        return false;
    }

    const char* current = first + 10;
    if (current == last) {
        return true;
    }
    if (*current != 'T' && *current != 't') {
        return false;
    }

    ++current;
    if (current + 5 > last || current[2] != ':') {
        return false;
    }

    int hour = 0;
    int minute = 0;
    if (!parse_digits<2>(current, hour) || !parse_digits<2>(current + 3, minute)) {
        return false;
    }
    if (hour > 23 || minute > 59) {
        return false;
    }

    current += 5;
    if (current != last && *current == ':') {
        ++current;
        if (current + 2 > last) {
            return false;
        }

        int second = 0;
        if (!parse_digits<2>(current, second) || second > 59) {
            return false;
        }

        current += 2;
        if (current != last && *current == '.') {
            ++current;
            const char* fraction_first = current;
            while (current != last && ascii_digit(*current)) {
                ++current;
            }
            if (current == fraction_first) {
                return false;
            }
        }
    }

    return consume_timezone(current, last);
}

namespace previous_data_type {

enum class DataType {
    unknown = -1,
    csv_null,
    csv_string,
    csv_int8,
    csv_int16,
    csv_int32,
    csv_int64,
    csv_bigint,
    csv_double
};

template<typename T>
inline long double pow10(T n) noexcept {
    long double multiplicand = n > 0 ? 10.0L : 0.1L;
    long double ret = 1.0L;
    T iterations = n > 0 ? n : static_cast<T>(0 - n);
    for (T i = 0; i < iterations; ++i) {
        ret *= multiplicand;
    }
    return ret;
}

inline DataType determine_integral_type(long double number) noexcept {
    if (number <= static_cast<long double>(std::numeric_limits<std::int8_t>::max())) {
        return DataType::csv_int8;
    }
    if (number <= static_cast<long double>(std::numeric_limits<std::int16_t>::max())) {
        return DataType::csv_int16;
    }
    if (number <= static_cast<long double>(std::numeric_limits<std::int32_t>::max())) {
        return DataType::csv_int32;
    }
    if (number <= static_cast<long double>(std::numeric_limits<std::int64_t>::max())) {
        return DataType::csv_int64;
    }
    return DataType::csv_bigint;
}

DataType data_type(std::string_view in, long double* out = nullptr, char decimal_symbol = '.') {
    if (in.empty()) {
        return DataType::csv_null;
    }

    bool ws_allowed = true;
    bool dot_allowed = true;
    bool digit_allowed = true;
    bool is_negative = false;
    bool has_digit = false;
    bool prob_float = false;

    unsigned places_after_decimal = 0;
    long double integral_part = 0;
    long double decimal_part = 0;

    for (std::size_t i = 0; i < in.size(); ++i) {
        const char current = in[i];

        switch (current) {
        case ' ':
            if (!ws_allowed) {
                if (i > 0 && std::isdigit(static_cast<unsigned char>(in[i - 1]))) {
                    digit_allowed = false;
                    ws_allowed = true;
                } else {
                    return DataType::csv_string;
                }
            }
            break;
        case '+':
            if (!ws_allowed) {
                return DataType::csv_string;
            }
            break;
        case '-':
            if (!ws_allowed) {
                return DataType::csv_string;
            }
            is_negative = true;
            break;
        case 'e':
        case 'E':
            if (prob_float || (i && i + 1 < in.size() && std::isdigit(static_cast<unsigned char>(in[i - 1])))) {
                std::size_t exponent_start_idx = i + 1;
                if (in[i + 1] == '+') {
                    ++exponent_start_idx;
                }

                long double exponent = 0;
                auto result = data_type(in.substr(exponent_start_idx), &exponent, decimal_symbol);
                if (result >= DataType::csv_int8 && result < DataType::csv_double) {
                    if (out) {
                        const auto coeff = is_negative ? -(integral_part + decimal_part) : integral_part + decimal_part;
                        *out = coeff * pow10(static_cast<long long>(exponent));
                    }
                    return DataType::csv_double;
                }
            }
            return DataType::csv_string;
        default:
            if (current >= '0' && current <= '9') {
                const short digit = static_cast<short>(current - '0');
                has_digit = true;
                if (!digit_allowed) {
                    return DataType::csv_string;
                }
                if (ws_allowed) {
                    ws_allowed = false;
                }
                if (prob_float) {
                    decimal_part += digit / pow10(++places_after_decimal);
                } else {
                    integral_part = (integral_part * 10.0L) + digit;
                }
            } else if (dot_allowed && current == decimal_symbol) {
                dot_allowed = false;
                prob_float = true;
            } else {
                return DataType::csv_string;
            }
        }
    }

    if (has_digit) {
        const long double number = integral_part + decimal_part;
        if (out) {
            *out = is_negative ? -number : number;
        }
        return prob_float ? DataType::csv_double : determine_integral_type(number);
    }

    return DataType::csv_null;
}

BenchKind classify(std::string_view value) {
    long double number = 0;
    const auto type = data_type(value, &number);
    if (type == DataType::csv_null) {
        return BenchKind::null_value;
    }
    if (type >= DataType::csv_int8 && type <= DataType::csv_bigint) {
        return BenchKind::integer;
    }
    if (type == DataType::csv_double) {
        return BenchKind::floating;
    }
    return BenchKind::string;
}

} // namespace previous_data_type

BenchKind classify_current(std::string_view value) {
    long double number = 0;
    std::int64_t integer = 0;
    bool boolean = false;
    const auto kind = classify_scalar::classify_scalar(
        value,
        classify_scalar::output_refs(number, integer, boolean));

    switch (kind) {
    case classify_scalar::scalar_null:
        return BenchKind::null_value;
    case classify_scalar::scalar_bool:
        return BenchKind::boolean;
    case classify_scalar::scalar_int:
        return BenchKind::integer;
    case classify_scalar::scalar_float:
        return BenchKind::floating;
    case classify_scalar::scalar_timestamp:
        return BenchKind::timestamp;
    case classify_scalar::scalar_string:
    default:
        return BenchKind::string;
    }
}

BenchKind classify_naive_from_chars(std::string_view value) {
    value = trim_ascii(value);
    if (value.empty()) {
        return BenchKind::null_value;
    }
    if (iequals(value, "true") || iequals(value, "false")) {
        return BenchKind::boolean;
    }
    if (naive_iso_timestamp(value)) {
        return BenchKind::timestamp;
    }

    std::int64_t integer = 0;
    const char* first = value.data();
    const char* last = first + value.size();
    auto int_result = std::from_chars(first, last, integer, 10);
    if (int_result.ec == std::errc{} && int_result.ptr == last) {
        return BenchKind::integer;
    }

    if (value.size() > 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
        int_result = std::from_chars(first + 2, last, integer, 16);
        if (int_result.ec == std::errc{} && int_result.ptr == last) {
            return BenchKind::integer;
        }
    } else if (value.size() > 3 && value[0] == '-' && value[1] == '0' && (value[2] == 'x' || value[2] == 'X')) {
        int_result = std::from_chars(first + 3, last, integer, 16);
        if (int_result.ec == std::errc{} && int_result.ptr == last) {
            return BenchKind::integer;
        }
    }

    double floating = 0;
    auto float_result = std::from_chars(first, last, floating);
    if (float_result.ec == std::errc{} && float_result.ptr == last && std::isfinite(floating)) {
        return std::trunc(floating) == floating ? BenchKind::integer : BenchKind::floating;
    }

    return BenchKind::string;
}

std::int64_t parse_int_classify_scalar(std::string_view value) {
    long double number = 0;
    std::int64_t integer = 0;
    bool boolean = false;
    const auto kind = classify_scalar::classify_numeric_scalar<classify_scalar::ScalarKind, false>(
        value,
        classify_scalar::output_refs(number, integer, boolean));

    return kind == classify_scalar::scalar_int ? integer : 0;
}

std::int64_t parse_int_from_chars(std::string_view value) {
    std::int64_t integer = 0;
    const char* first = value.data();
    const char* last = first + value.size();
    const auto result = std::from_chars(first, last, integer, 10);
    return result.ec == std::errc{} && result.ptr == last ? integer : 0;
}

double parse_float_classify_scalar(std::string_view value) {
    long double number = 0;
    std::int64_t integer = 0;
    bool boolean = false;
    const auto kind = classify_scalar::classify_numeric_scalar<classify_scalar::ScalarKind, false>(
        value,
        classify_scalar::output_refs(number, integer, boolean));

    return kind == classify_scalar::scalar_float || kind == classify_scalar::scalar_int
        ? static_cast<double>(number)
        : 0.0;
}

double parse_float_from_chars(std::string_view value) {
    double number = 0;
    const char* first = value.data();
    const char* last = first + value.size();
    const auto result = std::from_chars(first, last, number);
    return result.ec == std::errc{} && result.ptr == last && std::isfinite(number)
        ? number
        : 0.0;
}

template<typename Classifier>
void run_classifier(benchmark::State& state, Classifier classifier) {
    const auto& cases = mixed_cases();
    std::size_t i = 0;
    int sink = 0;

    for (auto _ : state) {
        const auto kind = classifier(cases[i].value);
        sink += static_cast<int>(kind);
        ++i;
        if (i == cases.size()) {
            i = 0;
        }
    }

    benchmark::DoNotOptimize(sink);
    state.SetItemsProcessed(state.iterations());
}

template<typename Parser, typename Value>
void run_parser(benchmark::State& state, const std::vector<bench_case>& cases, Parser parser, Value initial) {
    std::size_t i = 0;
    Value sink = initial;

    for (auto _ : state) {
        sink += parser(cases[i].value);
        ++i;
        if (i == cases.size()) {
            i = 0;
        }
    }

    benchmark::DoNotOptimize(sink);
    state.SetItemsProcessed(state.iterations());
}

} // namespace

static void BM_classify_scalar(benchmark::State& state) {
    run_classifier(state, classify_current);
}

static void BM_previous_data_type(benchmark::State& state) {
    run_classifier(state, previous_data_type::classify);
}

static void BM_naive_from_chars(benchmark::State& state) {
    run_classifier(state, classify_naive_from_chars);
}

static void BM_parse_int_classify_scalar(benchmark::State& state) {
    run_parser(state, int_parse_cases(), parse_int_classify_scalar, std::int64_t{0});
}

static void BM_parse_int_from_chars(benchmark::State& state) {
    run_parser(state, int_parse_cases(), parse_int_from_chars, std::int64_t{0});
}

static void BM_parse_float_classify_scalar(benchmark::State& state) {
    run_parser(state, float_parse_cases(), parse_float_classify_scalar, 0.0);
}

static void BM_parse_float_from_chars(benchmark::State& state) {
    run_parser(state, float_parse_cases(), parse_float_from_chars, 0.0);
}

BENCHMARK(BM_classify_scalar);
BENCHMARK(BM_previous_data_type);
BENCHMARK(BM_naive_from_chars);
BENCHMARK(BM_parse_int_classify_scalar);
BENCHMARK(BM_parse_int_from_chars);
BENCHMARK(BM_parse_float_classify_scalar);
BENCHMARK(BM_parse_float_from_chars);
