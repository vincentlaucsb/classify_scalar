#pragma once

#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>

#if defined(_MSVC_LANG)
#define CLASSIFY_SCALAR_CPLUSPLUS _MSVC_LANG
#else
#define CLASSIFY_SCALAR_CPLUSPLUS __cplusplus
#endif

#if CLASSIFY_SCALAR_CPLUSPLUS >= 202002L
#define CLASSIFY_SCALAR_HAS_CXX20
#endif

#if CLASSIFY_SCALAR_CPLUSPLUS >= 201703L
#define CLASSIFY_SCALAR_HAS_CXX17
#endif

#if CLASSIFY_SCALAR_CPLUSPLUS >= 201402L
#define CLASSIFY_SCALAR_HAS_CXX14
#endif

#if defined(__clang__) || defined(__GNUC__)
#define CLASSIFY_SCALAR_CONST __attribute__((__const__))
#else
#define CLASSIFY_SCALAR_CONST
#endif

#ifdef CLASSIFY_SCALAR_HAS_CXX17
#define IF_CONSTEXPR if constexpr
#define CONSTEXPR_VALUE constexpr
#define CONSTEXPR_17 constexpr
#else
#define IF_CONSTEXPR if
#define CONSTEXPR_VALUE const
#define CONSTEXPR_17 inline
#endif

#ifdef CLASSIFY_SCALAR_HAS_CXX14
#define CONSTEXPR_14 constexpr
#define CONSTEXPR_VALUE_14 constexpr
#else
#define CONSTEXPR_14 inline
#define CONSTEXPR_VALUE_14 const
#endif

#if defined(__GNUC__) && !defined(__clang__)
#if defined(CLASSIFY_SCALAR_HAS_CXX17) && (((__GNUC__ == 7) && (__GNUC_MINOR__ >= 2)) || (__GNUC__ >= 8))
#define CONSTEXPR constexpr
#endif
#else
#ifdef CLASSIFY_SCALAR_HAS_CXX17
#define CONSTEXPR constexpr
#endif
#endif

#ifndef CONSTEXPR
#define CONSTEXPR inline
#endif

#if CLASSIFY_SCALAR_CPLUSPLUS >= 201703L
#include <charconv>
#include <string_view>
#include <system_error>
#define CLASSIFY_SCALAR_HAS_STD_FROM_CHARS 1
#else
#define CLASSIFY_SCALAR_HAS_STD_FROM_CHARS 0
#endif

namespace classify_scalar {

#define CLASSIFY_SCALAR_BUILTIN_KINDS \
    scalar_null = 0,                  \
    scalar_string = 1,                \
    scalar_bool = 2,                  \
    scalar_int = 3,                   \
    scalar_float = 4

enum ScalarKind : int {
    CLASSIFY_SCALAR_BUILTIN_KINDS,
    scalar_invalid = -2,
    scalar_custom_begin = 1024
};

struct scalar_span {
    scalar_span() noexcept : first(nullptr), last(nullptr) {}
    scalar_span(const char* first_, const char* last_) noexcept : first(first_), last(last_) {}

    const char* first;
    const char* last;
};

struct scalar_outputs {
    scalar_outputs(
        long double* number_ = nullptr,
        std::int64_t* integer_ = nullptr,
        bool* boolean_ = nullptr) noexcept
        : number(number_), integer(integer_), boolean(boolean_) {}

    long double* number;
    std::int64_t* integer;
    bool* boolean;

    bool stores_values() const noexcept {
        return number || integer || boolean;
    }

    bool valid() const noexcept {
        return !stores_values() || (number && integer && boolean);
    }
};

enum class ParseFlag : unsigned char {
    other,
    space,
    digit,
    sign,
    decimal,
    might_be_exponential,
    might_be_hex_prefix,
    might_be_true,
    might_be_false,
    hex_digit
};

namespace detail {

inline bool is_ascii_space(const char c) noexcept {
    static CONSTEXPR_VALUE_14 bool table[256] = {
        false, false, false, false, false, false, false, false,
        false, true, true, true, true, true, false, false,
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        true
    };
    return table[static_cast<unsigned char>(c)];
}

template<bool RecognizeBool, bool RecognizeHex>
CLASSIFY_SCALAR_CONST CONSTEXPR_14 ParseFlag classify_ascii_char(const unsigned char c) noexcept {
    return c >= '0' && c <= '9' ? ParseFlag::digit
        : c == '+' || c == '-' ? ParseFlag::sign
        : c == '.' ? ParseFlag::decimal
        : c == 'e' || c == 'E' ? ParseFlag::might_be_exponential
        : RecognizeHex && (c == 'x' || c == 'X') ? ParseFlag::might_be_hex_prefix
        : RecognizeBool && (c == 't' || c == 'T') ? ParseFlag::might_be_true
        : RecognizeBool && (c == 'f' || c == 'F') ? ParseFlag::might_be_false
        : RecognizeHex && ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) ? ParseFlag::hex_digit
        : c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v' ? ParseFlag::space
        : ParseFlag::other;
}

template<std::size_t... Indexes>
struct index_sequence {};

template<std::size_t Count, std::size_t... Indexes>
struct make_index_sequence_impl : make_index_sequence_impl<Count - 1, Count - 1, Indexes...> {};

template<std::size_t... Indexes>
struct make_index_sequence_impl<0, Indexes...> {
    typedef index_sequence<Indexes...> type;
};

template<std::size_t Count>
struct make_index_sequence {
    typedef typename make_index_sequence_impl<Count>::type type;
};

struct parse_table_type {
    ParseFlag values[256];

    CONSTEXPR_14 ParseFlag operator[](unsigned char value) const noexcept {
        return values[value];
    }
};

template<bool RecognizeBool, bool RecognizeHex, std::size_t... Indexes>
CONSTEXPR_14 parse_table_type build_parse_table(index_sequence<Indexes...>) noexcept {
    return parse_table_type{{classify_ascii_char<RecognizeBool, RecognizeHex>(static_cast<unsigned char>(Indexes))...}};
}

template<bool RecognizeBool, bool RecognizeHex>
inline const parse_table_type& parse_table() noexcept {
    static CONSTEXPR_VALUE_14 parse_table_type table =
        build_parse_table<RecognizeBool, RecognizeHex>(typename make_index_sequence<256>::type());
    return table;
}

inline scalar_span trim_ascii(const char* first, const char* last) noexcept {
    while (first != last && is_ascii_space(*first)) {
        ++first;
    }

    while (first != last && is_ascii_space(*(last - 1))) {
        --last;
    }

    return scalar_span{first, last};
}

inline char ascii_lower(const char c) noexcept {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<char>(c - 'A' + 'a');
    }

    return c;
}

inline bool iequals(const char* first, const char* last, const char* expected) noexcept {
    const std::size_t size = static_cast<std::size_t>(last - first);
    const std::size_t expected_size = std::strlen(expected);
    if (size != expected_size) {
        return false;
    }

    for (std::size_t i = 0; i < size; ++i) {
        if (ascii_lower(first[i]) != expected[i]) {
            return false;
        }
    }

    return true;
}

inline bool parse_true(const char* first, const char* last, bool* out, bool store_values) noexcept {
    if (iequals(first, last, "true")) {
        if (store_values) {
            *out = true;
        }
        return true;
    }

    return false;
}

inline bool parse_false(const char* first, const char* last, bool* out, bool store_values) noexcept {
    if (iequals(first, last, "false")) {
        if (store_values) {
            *out = false;
        }
        return true;
    }

    return false;
}

inline int digit_value(const char c) noexcept {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }

    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }

    return -1;
}

CONSTEXPR_VALUE_14 std::int64_t int64_min_value = std::numeric_limits<std::int64_t>::min();
CONSTEXPR_VALUE_14 std::int64_t int64_max_value = std::numeric_limits<std::int64_t>::max();
CONSTEXPR_VALUE_14 std::uint64_t int64_positive_limit = static_cast<std::uint64_t>(int64_max_value);
CONSTEXPR_VALUE_14 std::uint64_t int64_negative_limit = int64_positive_limit + 1U;
CONSTEXPR_VALUE_14 long double int64_min_long_double = static_cast<long double>(int64_min_value);
CONSTEXPR_VALUE_14 long double int64_max_long_double = static_cast<long double>(int64_max_value);

inline bool fits_signed_accumulate(
    std::uint64_t& acc,
    const unsigned digit,
    const std::uint64_t limit) noexcept {
    if (acc > (limit - digit) / 10U) {
        return false;
    }

    acc = (acc * 10U) + digit;
    return true;
}

#if CLASSIFY_SCALAR_HAS_STD_FROM_CHARS
inline bool parse_decimal_integer_from_chars(
    const char* first,
    const char* last,
    std::int64_t* out,
    bool store_values) noexcept {
    std::int64_t parsed = 0;
    if (first != last && *first == '+') {
        ++first;
        if (first == last) {
            return false;
        }
    }

    const std::from_chars_result result = std::from_chars(first, last, parsed, 10);
    if (result.ec != std::errc() || result.ptr != last) {
        return false;
    }

    if (store_values) {
        *out = parsed;
    }
    return true;
}

inline bool parse_hex_integer_from_chars(
    const char* first,
    const char* last,
    std::int64_t* out,
    bool store_values) noexcept {
    if (first == last) {
        return false;
    }

    const char* current = first;
    bool negative = false;
    if (*current == '+' || *current == '-') {
        negative = *current == '-';
        ++current;
    }

    if (current + 2 > last || current[0] != '0' || (current[1] != 'x' && current[1] != 'X')) {
        return false;
    }
    current += 2;

    if (current == last) {
        return false;
    }

    std::uint64_t magnitude = 0;
    const std::from_chars_result result = std::from_chars(current, last, magnitude, 16);
    if (result.ec != std::errc() || result.ptr != last) {
        return false;
    }

    const std::uint64_t limit = negative ? int64_negative_limit : int64_positive_limit;
    if (magnitude > limit) {
        return false;
    }

    if (store_values) {
        if (negative) {
            *out = magnitude == limit
                ? int64_min_value
                : -static_cast<std::int64_t>(magnitude);
        } else {
            *out = static_cast<std::int64_t>(magnitude);
        }
    }

    return true;
}
#endif

inline bool parse_decimal_integer(
    const char* first,
    const char* last,
    std::int64_t* out,
    bool store_values) noexcept {
#if CLASSIFY_SCALAR_HAS_STD_FROM_CHARS
    return parse_decimal_integer_from_chars(first, last, out, store_values);
#else
    if (first == last) {
        return false;
    }

    bool negative = false;
    if (*first == '+' || *first == '-') {
        negative = *first == '-';
        ++first;
    }

    if (first == last) {
        return false;
    }

    const std::uint64_t limit = negative ? int64_negative_limit : int64_positive_limit;

    std::uint64_t acc = 0;
    for (const char* current = first; current != last; ++current) {
        const char c = *current;
        if (c < '0' || c > '9') {
            return false;
        }

        if (!fits_signed_accumulate(acc, static_cast<unsigned>(c - '0'), limit)) {
            return false;
        }
    }

    if (!store_values) {
        return true;
    }

    if (negative) {
        if (acc == limit) {
            *out = int64_min_value;
        } else {
            *out = -static_cast<std::int64_t>(acc);
        }
    } else {
        *out = static_cast<std::int64_t>(acc);
    }

    return true;
#endif
}

inline bool parse_hex_integer(
    const char* first,
    const char* last,
    std::int64_t* out,
    bool store_values) noexcept {
#if CLASSIFY_SCALAR_HAS_STD_FROM_CHARS
    return parse_hex_integer_from_chars(first, last, out, store_values);
#else
    if (first == last) {
        return false;
    }

    const char* current = first;
    bool negative = false;
    if (*current == '+' || *current == '-') {
        negative = *current == '-';
        ++current;
    }

    if (current + 2 > last || current[0] != '0' || (current[1] != 'x' && current[1] != 'X')) {
        return false;
    }
    current += 2;

    if (current == last) {
        return false;
    }

    const std::uint64_t limit = negative ? int64_negative_limit : int64_positive_limit;

    std::uint64_t acc = 0;
    for (; current != last; ++current) {
        const int digit = digit_value(*current);
        if (digit < 0) {
            return false;
        }

        if (acc > (limit - static_cast<unsigned>(digit)) / 16U) {
            return false;
        }
        acc = (acc * 16U) + static_cast<unsigned>(digit);
    }

    if (!store_values) {
        return true;
    }

    if (negative) {
        if (acc == limit) {
            *out = int64_min_value;
        } else {
            *out = -static_cast<std::int64_t>(acc);
        }
    } else {
        *out = static_cast<std::int64_t>(acc);
    }

    return true;
#endif
}

inline bool parse_floating(
    const char* first,
    const char* last,
    double* out,
    bool store_values) noexcept {
    const std::size_t size = static_cast<std::size_t>(last - first);
    if (first == last || size > 4096) {
        return false;
    }

#if CLASSIFY_SCALAR_HAS_STD_FROM_CHARS
    double parsed = 0;
    if (first != last && *first == '+') {
        ++first;
        if (first == last) {
            return false;
        }
    }

    const std::from_chars_result result = std::from_chars(first, last, parsed);
    if (result.ec != std::errc() || result.ptr != last || !std::isfinite(parsed)) {
        return false;
    }

    if (store_values) {
        *out = parsed;
    }

    return true;
#else
    char buffer[4097];
    std::size_t i = 0;
    for (const char* current = first; current != last; ++current, ++i) {
        const unsigned char c = static_cast<unsigned char>(*current);
        if (is_ascii_space(static_cast<char>(c))) {
            return false;
        }
        buffer[i] = static_cast<char>(c);
    }
    buffer[size] = '\0';

    char* parse_end = nullptr;
    errno = 0;
    const long double parsed = std::strtold(buffer, &parse_end);
    if (parse_end != buffer + size || errno == ERANGE || !std::isfinite(parsed)) {
        return false;
    }

    if (store_values) {
        *out = static_cast<double>(parsed);
    }

    return true;
#endif
}

inline bool floating_is_integral(const double value, std::int64_t* out, bool store_values) noexcept {
    if (value < int64_min_long_double || value > int64_max_long_double) {
        return false;
    }

    const std::int64_t integer = static_cast<std::int64_t>(value);
    if (static_cast<double>(integer) != value) {
        return false;
    }

    if (store_values) {
        *out = integer;
    }

    return true;
}

inline ScalarKind finish_integer(
    const std::int64_t parsed_integer,
    scalar_outputs outputs,
    bool store_values) noexcept {
    if (store_values) {
        *outputs.integer = parsed_integer;
        *outputs.number = static_cast<long double>(parsed_integer);
    }
    return scalar_int;
}

inline ScalarKind finish_floating(
    const double parsed_float,
    scalar_outputs outputs,
    bool store_values) noexcept {
    std::int64_t parsed_integer = 0;
    if (floating_is_integral(parsed_float, &parsed_integer, store_values)) {
        return finish_integer(parsed_integer, outputs, store_values);
    }

    if (store_values) {
        *outputs.number = parsed_float;
    }
    return scalar_float;
}

template<bool RecognizeBool = true, bool RecognizeHex = true>
struct builtin_parse_policy {
    ScalarKind on_decimal(
        const char* first,
        const char* last,
        const char*,
        scalar_outputs outputs,
        bool store_values) const noexcept {
        double parsed_float = 0;
        return parse_floating(first, last, &parsed_float, store_values)
            ? finish_floating(parsed_float, outputs, store_values)
            : scalar_string;
    }

    ScalarKind on_exponent(
        const char* first,
        const char* last,
        const char* current,
        scalar_outputs outputs,
        bool store_values) const noexcept {
        if (current == first || current + 1 == last) {
            return scalar_string;
        }

        double parsed_float = 0;
        return parse_floating(first, last, &parsed_float, store_values)
            ? finish_floating(parsed_float, outputs, store_values)
            : scalar_string;
    }

    ScalarKind on_hex_prefix(
        const char* first,
        const char* last,
        const char* current,
        scalar_outputs outputs,
        bool store_values) const noexcept {
        if (current == first || current + 1 == last) {
            return scalar_string;
        }

        std::int64_t parsed_integer = 0;
        return parse_hex_integer(first, last, &parsed_integer, store_values)
            ? finish_integer(parsed_integer, outputs, store_values)
            : scalar_string;
    }

    ScalarKind on_true(
        const char* first,
        const char* last,
        const char* current,
        scalar_outputs outputs,
        bool store_values) const noexcept {
        if (current != first) {
            return scalar_string;
        }

        return parse_true(first, last, outputs.boolean, store_values)
            ? scalar_bool
            : scalar_string;
    }

    ScalarKind on_false(
        const char* first,
        const char* last,
        const char* current,
        scalar_outputs outputs,
        bool store_values) const noexcept {
        if (current != first) {
            return scalar_string;
        }

        return parse_false(first, last, outputs.boolean, store_values)
            ? scalar_bool
            : scalar_string;
    }

    ScalarKind on_custom(
        ParseFlag,
        const char*,
        const char*,
        const char*,
        scalar_outputs,
        bool) const noexcept {
        return scalar_string;
    }

    ScalarKind on_space(
        const char*,
        const char*,
        const char*,
        scalar_outputs,
        bool) const noexcept {
        return scalar_string;
    }

    ScalarKind on_end(
        const char* first,
        const char* last,
        scalar_outputs outputs,
        bool store_values) const noexcept {
        std::int64_t parsed_integer = 0;
        return parse_decimal_integer(first, last, &parsed_integer, store_values)
            ? finish_integer(parsed_integer, outputs, store_values)
            : scalar_string;
    }
};

template<bool RecognizeBool, bool RecognizeHex, typename Policy>
inline ScalarKind classify_numeric_switch(
    const char* first,
    const char* last,
    scalar_outputs outputs,
    bool store_values,
    const Policy& policy) noexcept {
    for (const char* current = first; current != last; ++current) {
        const ParseFlag flag = parse_table<RecognizeBool, RecognizeHex>()[static_cast<unsigned char>(*current)];

        switch (flag) {
        case ParseFlag::digit:
        case ParseFlag::sign:
            break;
        case ParseFlag::decimal:
            return policy.on_decimal(first, last, current, outputs, store_values);
        case ParseFlag::might_be_exponential:
            return policy.on_exponent(first, last, current, outputs, store_values);
        case ParseFlag::might_be_hex_prefix:
            return policy.on_hex_prefix(first, last, current, outputs, store_values);
        case ParseFlag::might_be_true:
            return policy.on_true(first, last, current, outputs, store_values);
        case ParseFlag::might_be_false:
            return policy.on_false(first, last, current, outputs, store_values);
        case ParseFlag::hex_digit:
        case ParseFlag::other:
            return policy.on_custom(flag, first, last, current, outputs, store_values);
        case ParseFlag::space:
            return policy.on_space(first, last, current, outputs, store_values);
        }
    }

    return policy.on_end(first, last, outputs, store_values);
}

} // namespace detail

namespace detail {

template<
    bool RecognizeBool,
    bool RecognizeHex,
    typename Policy>
inline ScalarKind classify_scalar_trimmed(
    const char* first,
    const char* last,
    scalar_outputs outputs,
    bool store_values,
    const Policy& policy) noexcept {
    if (first == last) {
        return scalar_null;
    }

    return classify_numeric_switch<RecognizeBool, RecognizeHex>(
        first,
        last,
        outputs,
        store_values,
        policy);
}

} // namespace detail

template<
    bool TrimAsciiWhitespace = true,
    bool RecognizeBool = true,
    bool RecognizeHex = true,
    typename Policy = detail::builtin_parse_policy<RecognizeBool, RecognizeHex> >
inline ScalarKind classify_scalar(
    const char* first,
    const char* last,
    scalar_outputs outputs = scalar_outputs(),
    Policy policy = Policy()) noexcept {
    const bool store_values = outputs.stores_values();
    if (!outputs.valid()) {
        return scalar_invalid;
    }

    if (!first || !last || last < first) {
        return scalar_string;
    }

    const scalar_span span = TrimAsciiWhitespace
        ? detail::trim_ascii(first, last)
        : scalar_span{first, last};

    return detail::classify_scalar_trimmed<RecognizeBool, RecognizeHex>(
        span.first,
        span.last,
        outputs,
        store_values,
        policy);
}

inline ScalarKind classify_scalar(
    const char* first,
    const char* last,
    scalar_outputs outputs = scalar_outputs()) noexcept {
    return classify_scalar<true, true, true>(first, last, outputs, detail::builtin_parse_policy<true, true>());
}

template<
    bool TrimAsciiWhitespace = true,
    bool RecognizeBool = true,
    bool RecognizeHex = true,
    std::size_t Size,
    typename Policy = detail::builtin_parse_policy<RecognizeBool, RecognizeHex> >
inline ScalarKind classify_scalar(
    const char (&value)[Size],
    scalar_outputs outputs = scalar_outputs(),
    Policy policy = Policy()) noexcept {
    return classify_scalar<TrimAsciiWhitespace, RecognizeBool, RecognizeHex>(
        value,
        value + Size - 1,
        outputs,
        policy);
}

#if CLASSIFY_SCALAR_CPLUSPLUS >= 201703L
template<
    bool TrimAsciiWhitespace = true,
    bool RecognizeBool = true,
    bool RecognizeHex = true,
    typename Policy = detail::builtin_parse_policy<RecognizeBool, RecognizeHex> >
inline ScalarKind classify_scalar(
    std::string_view value,
    scalar_outputs outputs = scalar_outputs(),
    Policy policy = Policy()) noexcept {
    if (!outputs.valid()) {
        return scalar_invalid;
    }
    if (value.empty()) {
        return scalar_null;
    }

    return classify_scalar<TrimAsciiWhitespace, RecognizeBool, RecognizeHex>(
        value.data(),
        value.data() + value.size(),
        outputs,
        policy);
}

inline ScalarKind classify_scalar(
    std::string_view value,
    scalar_outputs outputs = scalar_outputs()) noexcept {
    return classify_scalar<true, true, true>(value, outputs, detail::builtin_parse_policy<true, true>());
}

#endif

} // namespace classify_scalar
