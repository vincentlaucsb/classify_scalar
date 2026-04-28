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
#if defined(_LIBCPP_VERSION)
#define CLASSIFY_SCALAR_HAS_STD_FLOAT_FROM_CHARS 0
#else
#define CLASSIFY_SCALAR_HAS_STD_FLOAT_FROM_CHARS 1
#endif
#else
#define CLASSIFY_SCALAR_HAS_STD_FROM_CHARS 0
#define CLASSIFY_SCALAR_HAS_STD_FLOAT_FROM_CHARS 0
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

struct classify_only_output {
    void set_integer(std::int64_t) const noexcept {}
    void set_number(long double) const noexcept {}
    void set_bool(bool) const noexcept {}
};

struct builtin_output_refs {
    builtin_output_refs(long double& number_, std::int64_t& integer_, bool& boolean_) noexcept
        : number(number_), integer(integer_), boolean(boolean_) {}

    void set_integer(std::int64_t value) const noexcept {
        integer = value;
        number = static_cast<long double>(value);
    }

    void set_number(long double value) const noexcept {
        number = value;
    }

    void set_bool(bool value) const noexcept {
        boolean = value;
    }

    long double& number;
    std::int64_t& integer;
    bool& boolean;
};

inline builtin_output_refs output_refs(long double& number, std::int64_t& integer, bool& boolean) noexcept {
    return builtin_output_refs(number, integer, boolean);
}

enum class ParseFlag : unsigned char {
    /// Any byte with no scalar-classification meaning in the active parse table.
    other,
    /// ASCII whitespace bytes: space, tab, LF, CR, FF, and VT.
    space,
    /// ASCII decimal digits '0' through '9'.
    digit,
    /// Decimal point byte '.'.
    decimal,
    /// Exponent marker bytes 'e' and 'E'.
    might_be_exponential,
    /// Hexadecimal prefix marker bytes 'x' and 'X' when hex recognition is enabled.
    might_be_hex_prefix,
    /// Hexadecimal digit bytes 'a' through 'f' and 'A' through 'F' when hex recognition is enabled.
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

template<bool RecognizeHex>
CLASSIFY_SCALAR_CONST CONSTEXPR_14 ParseFlag classify_ascii_char(const unsigned char c) noexcept {
    return c >= '0' && c <= '9' ? ParseFlag::digit
        : c == '.' ? ParseFlag::decimal
        : c == 'e' || c == 'E' ? ParseFlag::might_be_exponential
        : RecognizeHex && (c == 'x' || c == 'X') ? ParseFlag::might_be_hex_prefix
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

struct parse_state {
    parse_state(const char* first_, const char* last_) noexcept
        : first(first_),
          last(last_),
          current(first_),
          numeric_first(first_),
          sign(nullptr),
          leading_sign(false),
          leading_plus(false),
          negative(false) {}

    const char* first;
    const char* last;
    const char* current;
    const char* numeric_first;
    const char* sign;
    bool leading_sign;
    bool leading_plus;
    bool negative;
};

template<bool RecognizeHex, std::size_t... Indexes>
CONSTEXPR_14 parse_table_type build_parse_table(index_sequence<Indexes...>) noexcept {
    return parse_table_type{{classify_ascii_char<RecognizeHex>(static_cast<unsigned char>(Indexes))...}};
}

template<bool RecognizeHex>
inline const parse_table_type& parse_table() noexcept {
    static CONSTEXPR_VALUE_14 parse_table_type table =
        build_parse_table<RecognizeHex>(typename make_index_sequence<256>::type());
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

inline bool parse_true(const char* first, const char* last, bool* out) noexcept {
    if (iequals(first, last, "true")) {
        if (out) {
            *out = true;
        }
        return true;
    }

    return false;
}

inline bool parse_false(const char* first, const char* last, bool* out) noexcept {
    if (iequals(first, last, "false")) {
        if (out) {
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
    const parse_state& state,
    std::int64_t* out) noexcept {
    const char* first = state.numeric_first;
    const char* last = state.last;
    std::int64_t parsed = 0;
    if (first == last) {
        return false;
    }

    const std::from_chars_result result = std::from_chars(first, last, parsed, 10);
    if (result.ec != std::errc() || result.ptr != last) {
        return false;
    }

    if (out) {
        *out = parsed;
    }
    return true;
}

inline bool parse_hex_integer_from_chars(
    const parse_state& state,
    std::int64_t* out) noexcept {
    const char* current = state.leading_sign ? state.first + 1 : state.first;
    const char* last = state.last;
    if (current == last) {
        return false;
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

    const std::uint64_t limit = state.negative ? int64_negative_limit : int64_positive_limit;
    if (magnitude > limit) {
        return false;
    }

    if (out) {
        if (state.negative) {
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
    const parse_state& state,
    std::int64_t* out) noexcept {
#if CLASSIFY_SCALAR_HAS_STD_FROM_CHARS
    return parse_decimal_integer_from_chars(state, out);
#else
    const char* first = state.leading_sign ? state.first + 1 : state.first;
    const char* last = state.last;
    if (first == last) {
        return false;
    }

    const std::uint64_t limit = state.negative ? int64_negative_limit : int64_positive_limit;

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

    if (!out) {
        return true;
    }

    if (state.negative) {
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
    const parse_state& state,
    std::int64_t* out) noexcept {
#if CLASSIFY_SCALAR_HAS_STD_FROM_CHARS
    return parse_hex_integer_from_chars(state, out);
#else
    const char* current = state.leading_sign ? state.first + 1 : state.first;
    const char* last = state.last;
    if (current == last) {
        return false;
    }

    if (current + 2 > last || current[0] != '0' || (current[1] != 'x' && current[1] != 'X')) {
        return false;
    }
    current += 2;

    if (current == last) {
        return false;
    }

    const std::uint64_t limit = state.negative ? int64_negative_limit : int64_positive_limit;

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

    if (!out) {
        return true;
    }

    if (state.negative) {
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
    const parse_state& state,
    double* out) noexcept {
    const char* first = state.numeric_first;
    const char* last = state.last;
    const std::size_t size = static_cast<std::size_t>(last - first);
    if (first == last || size > 4096) {
        return false;
    }

#if CLASSIFY_SCALAR_HAS_STD_FLOAT_FROM_CHARS
    double parsed = 0;
    const std::from_chars_result result = std::from_chars(first, last, parsed);
    if (result.ec != std::errc() || result.ptr != last || !std::isfinite(parsed)) {
        return false;
    }

    if (out) {
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

    if (out) {
        *out = static_cast<double>(parsed);
    }

    return true;
#endif
}

inline bool floating_is_integral(const double value, std::int64_t* out) noexcept {
    if (value < int64_min_long_double || value > int64_max_long_double) {
        return false;
    }

    const std::int64_t integer = static_cast<std::int64_t>(value);
    if (static_cast<double>(integer) != value) {
        return false;
    }

    if (out) {
        *out = integer;
    }

    return true;
}

inline ScalarKind finish_integer(
    const std::int64_t,
    classify_only_output&) noexcept {
    return scalar_int;
}

template<typename Output>
inline ScalarKind finish_integer(
    const std::int64_t parsed_integer,
    Output& output) noexcept {
    output.set_integer(parsed_integer);
    return scalar_int;
}

inline ScalarKind finish_floating(
    const double parsed_float,
    classify_only_output&) noexcept {
    if (floating_is_integral(parsed_float, nullptr)) {
        return scalar_int;
    }

    return scalar_float;
}

template<typename Output>
inline ScalarKind finish_floating(
    const double parsed_float,
    Output& output) noexcept {
    std::int64_t parsed_integer = 0;
    if (floating_is_integral(parsed_float, &parsed_integer)) {
        return finish_integer(parsed_integer, output);
    }

    output.set_number(parsed_float);
    return scalar_float;
}

template<bool RecognizeBool = true, bool RecognizeHex = true>
struct builtin_parse_policy {
    template<typename Output>
    ScalarKind on_decimal(
        parse_state& state,
        Output& output) const noexcept {
        double parsed_float = 0;
        return parse_floating(state, &parsed_float)
            ? finish_floating(parsed_float, output)
            : scalar_string;
    }

    template<typename Output>
    ScalarKind on_exponent(
        parse_state& state,
        Output& output) const noexcept {
        if (state.current == state.first || state.current + 1 == state.last) {
            return scalar_string;
        }

        double parsed_float = 0;
        return parse_floating(state, &parsed_float)
            ? finish_floating(parsed_float, output)
            : scalar_string;
    }

    template<typename Output>
    ScalarKind on_hex_prefix(
        parse_state& state,
        Output& output) const noexcept {
        if (state.current == state.first || state.current + 1 == state.last) {
            return scalar_string;
        }

        std::int64_t parsed_integer = 0;
        return parse_hex_integer(state, &parsed_integer)
            ? finish_integer(parsed_integer, output)
            : scalar_string;
    }

    template<typename Output>
    ScalarKind on_true(
        parse_state& state,
        Output& output) const noexcept {
        if (state.current != state.first) {
            return scalar_string;
        }

        bool parsed = false;
        if (!parse_true(state.first, state.last, &parsed)) {
            return scalar_string;
        }
        output.set_bool(parsed);
        return scalar_bool;
    }

    template<typename Output>
    ScalarKind on_false(
        parse_state& state,
        Output& output) const noexcept {
        if (state.current != state.first) {
            return scalar_string;
        }

        bool parsed = false;
        if (!parse_false(state.first, state.last, &parsed)) {
            return scalar_string;
        }
        output.set_bool(parsed);
        return scalar_bool;
    }

    template<typename Output>
    ScalarKind on_custom(
        ParseFlag,
        parse_state&,
        Output&) const noexcept {
        return scalar_string;
    }

    template<typename Output>
    ScalarKind on_space(
        parse_state&,
        Output&) const noexcept {
        return scalar_string;
    }

    template<typename Output>
    ScalarKind on_end(
        parse_state& state,
        Output& output) const noexcept {
        std::int64_t parsed_integer = 0;
        return parse_decimal_integer(state, &parsed_integer)
            ? finish_integer(parsed_integer, output)
            : scalar_string;
    }
};

template<bool RecognizeBool, bool RecognizeHex, typename Output, typename Policy>
inline ScalarKind classify_numeric_switch(
    const char* first,
    const char* last,
    Output& output,
    const Policy& policy) noexcept {
    parse_state state(first, last);

    if (*first == '+' || *first == '-') {
        state.sign = first;
        state.leading_sign = true;
        state.leading_plus = *first == '+';
        state.numeric_first = state.leading_plus ? first + 1 : first;
        state.negative = *first == '-';
    }

    IF_CONSTEXPR (RecognizeBool) {
        state.current = first;
        switch (*first) {
        case 't':
        case 'T':
            return policy.on_true(state, output);
        case 'f':
        case 'F':
            return policy.on_false(state, output);
        default:
            break;
        }
    }

    const char* scan_first = state.leading_sign ? first + 1 : first;
    for (const char* current = scan_first; current != last; ++current) {
        state.current = current;
        const ParseFlag flag = parse_table<RecognizeHex>()[static_cast<unsigned char>(*current)];

        switch (flag) {
        case ParseFlag::digit:
            break;
        case ParseFlag::decimal:
            return policy.on_decimal(state, output);
        case ParseFlag::might_be_exponential:
            return policy.on_exponent(state, output);
        case ParseFlag::might_be_hex_prefix:
            return policy.on_hex_prefix(state, output);
        case ParseFlag::hex_digit:
        case ParseFlag::other:
            return policy.on_custom(flag, state, output);
        case ParseFlag::space:
            return policy.on_space(state, output);
        }
    }

    state.current = last;
    return policy.on_end(state, output);
}

} // namespace detail

namespace detail {

template<
    bool RecognizeBool,
    bool RecognizeHex,
    typename Output,
    typename Policy>
inline ScalarKind classify_scalar_trimmed(
    const char* first,
    const char* last,
    Output& output,
    const Policy& policy) noexcept {
    if (first == last) {
        return scalar_null;
    }

    return classify_numeric_switch<RecognizeBool, RecognizeHex>(
        first,
        last,
        output,
        policy);
}

} // namespace detail

template<
    bool TrimAsciiWhitespace = true,
    bool RecognizeBool = true,
    bool RecognizeHex = true,
    typename Output = classify_only_output,
    typename Policy = detail::builtin_parse_policy<RecognizeBool, RecognizeHex> >
inline ScalarKind classify_scalar(
    const char* first,
    const char* last,
    Output output = Output(),
    Policy policy = Policy()) noexcept {
    if (!first || !last || last < first) {
        return scalar_string;
    }

    const scalar_span span = TrimAsciiWhitespace
        ? detail::trim_ascii(first, last)
        : scalar_span{first, last};

    return detail::classify_scalar_trimmed<RecognizeBool, RecognizeHex>(
        span.first,
        span.last,
        output,
        policy);
}

inline ScalarKind classify_scalar(
    const char* first,
    const char* last,
    classify_only_output output = classify_only_output()) noexcept {
    return classify_scalar<true, true, true>(first, last, output, detail::builtin_parse_policy<true, true>());
}

template<
    bool TrimAsciiWhitespace = true,
    bool RecognizeBool = true,
    bool RecognizeHex = true,
    std::size_t Size,
    typename Output = classify_only_output,
    typename Policy = detail::builtin_parse_policy<RecognizeBool, RecognizeHex> >
inline ScalarKind classify_scalar(
    const char (&value)[Size],
    Output output = Output(),
    Policy policy = Policy()) noexcept {
    return classify_scalar<TrimAsciiWhitespace, RecognizeBool, RecognizeHex>(
        value,
        value + Size - 1,
        output,
        policy);
}

#if CLASSIFY_SCALAR_CPLUSPLUS >= 201703L
template<
    bool TrimAsciiWhitespace = true,
    bool RecognizeBool = true,
    bool RecognizeHex = true,
    typename Output = classify_only_output,
    typename Policy = detail::builtin_parse_policy<RecognizeBool, RecognizeHex> >
inline ScalarKind classify_scalar(
    std::string_view value,
    Output output = Output(),
    Policy policy = Policy()) noexcept {
    if (value.empty()) {
        return scalar_null;
    }

    return classify_scalar<TrimAsciiWhitespace, RecognizeBool, RecognizeHex>(
        value.data(),
        value.data() + value.size(),
        output,
        policy);
}

inline ScalarKind classify_scalar(
    std::string_view value,
    classify_only_output output = classify_only_output()) noexcept {
    return classify_scalar<true, true, true>(value, output, detail::builtin_parse_policy<true, true>());
}

#endif

} // namespace classify_scalar
