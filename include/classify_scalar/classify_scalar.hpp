#pragma once

#include <array>
#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <tuple>
#include <type_traits>

#if defined(_MSVC_LANG)
#define CLASSIFY_SCALAR_CPLUSPLUS _MSVC_LANG
#else
#define CLASSIFY_SCALAR_CPLUSPLUS __cplusplus
#endif

#if CLASSIFY_SCALAR_CPLUSPLUS >= 201703L
#define CLASSIFY_SCALAR_HAS_CXX17
#endif

#if CLASSIFY_SCALAR_CPLUSPLUS >= 201402L
#define CLASSIFY_SCALAR_HAS_CXX14
#endif

#if defined(_WIN32) || defined(__LITTLE_ENDIAN__) \
    || (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
#define CLASSIFY_SCALAR_TRUE_U32 0x65757274U
#define CLASSIFY_SCALAR_FALSE_PREFIX_U32 0x736c6166U
#else
#define CLASSIFY_SCALAR_TRUE_U32 0x74727565U
#define CLASSIFY_SCALAR_FALSE_PREFIX_U32 0x66616c73U
#endif

#if defined(__clang__) || defined(__GNUC__)
#define CLASSIFY_SCALAR_CONST __attribute__((__const__))
#else
#define CLASSIFY_SCALAR_CONST
#endif

#if defined(_MSC_VER)
#define CLASSIFY_SCALAR_FORCE_INLINE __forceinline
#elif defined(__clang__) || defined(__GNUC__)
#define CLASSIFY_SCALAR_FORCE_INLINE inline __attribute__((__always_inline__))
#else
#define CLASSIFY_SCALAR_FORCE_INLINE inline
#endif

#ifdef CLASSIFY_SCALAR_HAS_CXX14
#define CONSTEXPR_14 constexpr
#define CONSTEXPR_VALUE_14 constexpr
#else
#define CONSTEXPR_14 inline
#define CONSTEXPR_VALUE_14 const
#endif

#if CLASSIFY_SCALAR_CPLUSPLUS >= 201703L
#include <string_view>
#include <system_error>
#if !defined(CLASSIFY_SCALAR_DISABLE_STD_FROM_CHARS)
#include <charconv>
#if defined(_LIBCPP_VERSION)
#define CLASSIFY_SCALAR_HAS_STD_FLOAT_FROM_CHARS 0
#else
#define CLASSIFY_SCALAR_HAS_STD_FLOAT_FROM_CHARS 1
#endif
#else
#define CLASSIFY_SCALAR_HAS_STD_FLOAT_FROM_CHARS 0
#endif
#else
#define CLASSIFY_SCALAR_HAS_STD_FLOAT_FROM_CHARS 0
#endif

namespace classify_scalar {

enum ScalarKind : int {
    scalar_null = 0,
    scalar_string = 1,
    scalar_bool = 2,
    scalar_int = 3,
    scalar_float = 4,
    scalar_timestamp = 5,
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

CLASSIFY_SCALAR_FORCE_INLINE builtin_output_refs output_refs(long double& number, std::int64_t& integer, bool& boolean) noexcept {
    return builtin_output_refs(number, integer, boolean);
}

enum class ParseFlag : unsigned char {
    /// Any byte with no scalar-classification meaning in the active parse table.
    other,
    /// Decimal point byte '.'.
    decimal,
    /// Exponent marker bytes 'e' and 'E'.
    might_be_exponential,
    /// Hexadecimal prefix marker bytes 'x' and 'X' when hex recognition is enabled.
    might_be_hex_prefix
};

namespace detail {

CLASSIFY_SCALAR_FORCE_INLINE bool is_ascii_space(const char c) noexcept {
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
    return c == '.' ? ParseFlag::decimal
        : c == 'e' || c == 'E' ? ParseFlag::might_be_exponential
        : RecognizeHex && (c == 'x' || c == 'X') ? ParseFlag::might_be_hex_prefix
        : ParseFlag::other;
}

CLASSIFY_SCALAR_CONST CONSTEXPR_14 char ascii_lower_char(const unsigned char c) noexcept {
    return static_cast<char>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
}

CLASSIFY_SCALAR_CONST CONSTEXPR_14 bool ascii_digit_value(const unsigned char c) noexcept {
    return c >= '0' && c <= '9';
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

struct dispatch_table_type {
    unsigned char values[256];

    CONSTEXPR_14 unsigned char operator[](unsigned char value) const noexcept {
        return values[value];
    }
};

struct parse_state {
    enum Sign : unsigned char {
        no_sign,
        positive_sign,
        negative_sign
    };

    parse_state(const char* first_, const char* last_) noexcept
        : first(first_),
          last(last_),
          current(first_),
          numeric_first(first_),
          sign(no_sign) {}

    const char* first;
    const char* last;
    const char* current;
    const char* numeric_first;
    Sign sign;
};

CONSTEXPR_VALUE_14 unsigned char no_dispatch_policy = 255U;

template<unsigned char Index, typename... Policies>
struct dispatch_index_impl;

template<unsigned char Index>
struct dispatch_index_impl<Index> {
    CLASSIFY_SCALAR_CONST CONSTEXPR_14 static unsigned char value(unsigned char) noexcept {
        return no_dispatch_policy;
    }
};

template<unsigned char Index, typename First, typename... Rest>
struct dispatch_index_impl<Index, First, Rest...> {
    CLASSIFY_SCALAR_CONST CONSTEXPR_14 static unsigned char value(unsigned char c) noexcept {
        return First::matches_leading(c)
            ? Index
            : dispatch_index_impl<static_cast<unsigned char>(Index + 1U), Rest...>::value(c);
    }
};

template<std::size_t Index, std::size_t Count>
struct policy_dispatch_impl {
    template<typename Tuple, typename Output>
    CLASSIFY_SCALAR_FORCE_INLINE static ScalarKind call(
        const unsigned char policy_index,
        const Tuple& policies,
        parse_state& state,
        Output& output) noexcept {
        typedef typename std::remove_reference<Tuple>::type tuple_type;
        typedef typename std::tuple_element<Index, tuple_type>::type policy_type;

        if (policy_index > Index)
            return policy_dispatch_impl<Index + 1U, Count>::call(policy_index, policies, state, output);

        if (policy_index == Index || policy_type::matches_leading(static_cast<unsigned char>(*state.first))) {
            const ScalarKind kind = std::get<Index>(policies).on_dispatch(state, output);
            if (kind != scalar_string)
                return kind;
        }

        return policy_dispatch_impl<Index + 1U, Count>::call(policy_index, policies, state, output);
    }
};

template<std::size_t Count>
struct policy_dispatch_impl<Count, Count> {
    template<typename Tuple, typename Output>
    CLASSIFY_SCALAR_FORCE_INLINE static ScalarKind call(
        unsigned char,
        const Tuple&,
        parse_state&,
        Output&) noexcept {
        return scalar_string;
    }
};

template<typename... Policies>
struct policy_pack {
    typedef std::tuple<Policies...> tuple_type;

    policy_pack() noexcept : policies() {}

    explicit policy_pack(Policies... policies_) noexcept
        : policies(policies_...) {}

    CLASSIFY_SCALAR_CONST CONSTEXPR_14 static unsigned char dispatch_index(unsigned char c) noexcept {
        return dispatch_index_impl<0U, Policies...>::value(c);
    }

    template<typename Output>
    CLASSIFY_SCALAR_FORCE_INLINE ScalarKind dispatch(
        const unsigned char policy_index,
        parse_state& state,
        Output& output) const noexcept {
        return policy_dispatch_impl<0U, sizeof...(Policies)>::call(policy_index, policies, state, output);
    }

    tuple_type policies;
};

template<bool RecognizeHex, std::size_t... Indexes>
CONSTEXPR_14 parse_table_type build_parse_table(index_sequence<Indexes...>) noexcept {
    return parse_table_type{{classify_ascii_char<RecognizeHex>(static_cast<unsigned char>(Indexes))...}};
}

template<bool RecognizeHex>
CLASSIFY_SCALAR_FORCE_INLINE const parse_table_type& parse_table() noexcept {
    static CONSTEXPR_VALUE_14 parse_table_type table =
        build_parse_table<RecognizeHex>(typename make_index_sequence<256>::type());
    return table;
}

template<typename PolicyPack, std::size_t... Indexes>
CONSTEXPR_14 dispatch_table_type build_dispatch_table(index_sequence<Indexes...>) noexcept {
    return dispatch_table_type{{PolicyPack::dispatch_index(static_cast<unsigned char>(Indexes))...}};
}

template<typename PolicyPack>
CLASSIFY_SCALAR_FORCE_INLINE const dispatch_table_type& dispatch_table() noexcept {
    static CONSTEXPR_VALUE_14 dispatch_table_type table =
        build_dispatch_table<PolicyPack>(typename make_index_sequence<256>::type());
    return table;
}

CONSTEXPR_14 std::array<bool, 256> create_ascii_digits_table() noexcept {
    std::array<bool, 256> table = {};
    for (std::size_t i = 0; i < table.size(); ++i) {
        table[i] = ascii_digit_value(static_cast<unsigned char>(i));
    }
    return table;
}

CLASSIFY_SCALAR_FORCE_INLINE std::uint32_t load_u32(const char* value) noexcept {
    std::uint32_t word = 0;
    std::memcpy(&word, value, sizeof(word));
    return word;
}

CLASSIFY_SCALAR_FORCE_INLINE bool is_ascii_digit(const unsigned char c) noexcept {
    static const std::array<bool, 256> ascii_digits = create_ascii_digits_table();
    return ascii_digits[c];
}

CLASSIFY_SCALAR_FORCE_INLINE scalar_span trim_ascii(const char* first, const char* last) noexcept {
    while (first != last && is_ascii_space(*first))
        ++first;

    while (first != last && is_ascii_space(*(last - 1)))
        --last;

    return scalar_span{first, last};
}

CLASSIFY_SCALAR_FORCE_INLINE bool parse_true(const char* first, const char* last, bool* out) noexcept {
    if (last - first != 4)
        return false;

    const std::uint32_t lowered = load_u32(first) | 0x20202020U;
    if (lowered != CLASSIFY_SCALAR_TRUE_U32)
        return false;

    if (out)
        *out = true;

    return true;
}

CLASSIFY_SCALAR_FORCE_INLINE bool parse_false(const char* first, const char* last, bool* out) noexcept {
    if (last - first != 5)
        return false;

    const std::uint32_t lowered = load_u32(first) | 0x20202020U;
    if (lowered != CLASSIFY_SCALAR_FALSE_PREFIX_U32 || (static_cast<unsigned char>(first[4]) | 0x20U) != 'e')
        return false;

    if (out)
        *out = false;

    return true;
}

CONSTEXPR_VALUE_14 unsigned char invalid_digit_value = 255U;

CONSTEXPR_14 std::array<unsigned char, 256> create_digit_values_table() noexcept {
    std::array<unsigned char, 256> table = {};
    for (std::size_t i = 0; i < table.size(); ++i) {
        table[i] = invalid_digit_value;
    }
    for (unsigned char i = 0; i < 10; ++i) {
        table[static_cast<unsigned char>('0' + i)] = i;
    }
    for (unsigned char i = 0; i < 26; ++i) {
        table[static_cast<unsigned char>('A' + i)] = static_cast<unsigned char>(10U + i);
        table[static_cast<unsigned char>('a' + i)] = static_cast<unsigned char>(10U + i);
    }
    return table;
}

CLASSIFY_SCALAR_FORCE_INLINE unsigned char digit_value(const char c) noexcept {
    static const std::array<unsigned char, 256> digit_values = create_digit_values_table();
    return digit_values[static_cast<unsigned char>(c)];
}

CLASSIFY_SCALAR_CONST CONSTEXPR_14 bool is_leap_year(const int year) noexcept {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

CONSTEXPR_VALUE_14 int common_days_in_month[13] = {
    0,
    31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
};

CLASSIFY_SCALAR_CONST CONSTEXPR_14 int days_in_month(const int year, const int month) noexcept {
    return month == 2 && is_leap_year(year) ? 29 : common_days_in_month[month];
}

template<std::size_t Count>
CLASSIFY_SCALAR_FORCE_INLINE bool parse_digits(const char* value, int& out) noexcept {
    int parsed = 0;
    for (std::size_t i = 0; i < Count; ++i) {
        const unsigned char c = static_cast<unsigned char>(value[i]);
        if (!is_ascii_digit(c))
            return false;

        parsed = (parsed * 10) + (value[i] - '0');
    }
    out = parsed;
    return true;
}

CLASSIFY_SCALAR_FORCE_INLINE bool valid_iso_date(const int year, const int month, const int day) noexcept {
    return (month < 1 || month > 12) ? false :
        day >= 1 && day <= days_in_month(year, month);
}

CLASSIFY_SCALAR_FORCE_INLINE bool consume_iso_timezone(const char*& current, const char* last) noexcept {
    if (current == last)
        return true;

    if (ascii_lower_char(static_cast<unsigned char>(*current)) == 'z') {
        ++current;
        return current == last;
    }

    if (*current != '+' && *current != '-')
        return false;

    if (current + 6 != last || current[3] != ':')
        return false;

    int hour = 0;
    int minute = 0;
    if (!parse_digits<2>(current + 1, hour) || !parse_digits<2>(current + 4, minute))
        return false;

    current = last;
    return hour <= 23 && minute <= 59;
}

CLASSIFY_SCALAR_FORCE_INLINE bool parse_iso_timestamp(const char* first, const char* last) noexcept {
    if (last - first < 10)
        return false;

    if (first[4] != '-' || first[7] != '-')
        return false;

    int year = 0;
    int month = 0;
    int day = 0;
    if (!parse_digits<4>(first, year) || !parse_digits<2>(first + 5, month) || !parse_digits<2>(first + 8, day))
        return false;

    if (!valid_iso_date(year, month, day))
        return false;

    const char* current = first + 10;
    if (current == last)
        return true;

    if (ascii_lower_char(static_cast<unsigned char>(*current)) != 't')
        return false;

    ++current;
    if (current + 5 > last || current[2] != ':')
        return false;

    int hour = 0;
    int minute = 0;
    if (!parse_digits<2>(current, hour) || !parse_digits<2>(current + 3, minute))
        return false;

    if (hour > 23 || minute > 59)
        return false;

    current += 5;
    if (current != last && *current == ':') {
        ++current;
        if (current + 2 > last)
            return false;

        int second = 0;
        if (!parse_digits<2>(current, second) || second > 59)
            return false;

        current += 2;
        if (current != last && *current == '.') {
            ++current;
            const char* fraction_first = current;
            while (current != last && is_ascii_digit(static_cast<unsigned char>(*current)))
                ++current;

            if (current == fraction_first)
                return false;
        }
    }

    return consume_iso_timezone(current, last);
}

CONSTEXPR_VALUE_14 std::int64_t int64_min_value = std::numeric_limits<std::int64_t>::min();
CONSTEXPR_VALUE_14 std::int64_t int64_max_value = std::numeric_limits<std::int64_t>::max();
CONSTEXPR_VALUE_14 std::uint64_t int64_positive_limit = static_cast<std::uint64_t>(int64_max_value);
CONSTEXPR_VALUE_14 std::uint64_t int64_negative_limit = int64_positive_limit + 1U;
CONSTEXPR_VALUE_14 long double int64_min_long_double = static_cast<long double>(int64_min_value);
CONSTEXPR_VALUE_14 long double int64_max_long_double = static_cast<long double>(int64_max_value);

CLASSIFY_SCALAR_FORCE_INLINE bool finish_signed_integer(
    const parse_state& state,
    const std::uint64_t magnitude,
    const std::uint64_t limit,
    std::int64_t* out) noexcept {
    if (magnitude > limit)
        return false;

    if (out) {
        if (state.sign == parse_state::negative_sign) {
            *out = magnitude == limit
                ? int64_min_value
                : -static_cast<std::int64_t>(magnitude);
        } else {
            *out = static_cast<std::int64_t>(magnitude);
        }
    }

    return true;
}

CLASSIFY_SCALAR_FORCE_INLINE bool parse_integer_digits(
    const parse_state& state,
    const char* first,
    const char* last,
    const unsigned base,
    std::int64_t* out) noexcept {
    assert(first != last);

    const std::uint64_t limit = state.sign == parse_state::negative_sign ? int64_negative_limit : int64_positive_limit;

    std::uint64_t acc = 0;
    for (const char* current = first; current != last; ++current) {
        const unsigned char digit = digit_value(*current);
        if (digit >= base)
            return false;

        if (acc > (limit - digit) / base)
            return false;

        acc = (acc * base) + digit;
    }

    return finish_signed_integer(state, acc, limit, out);
}

CLASSIFY_SCALAR_FORCE_INLINE bool parse_decimal_integer(
    const parse_state& state,
    std::int64_t* out) noexcept {
    const char* first = state.sign == parse_state::no_sign ? state.first : state.first + 1;
    const char* last = state.last;
    return parse_integer_digits(state, first, last, 10U, out);
}

CLASSIFY_SCALAR_FORCE_INLINE bool parse_hex_integer(
    const parse_state& state,
    std::int64_t* out) noexcept {
    const char* current = state.sign == parse_state::no_sign ? state.first : state.first + 1;
    const char* last = state.last;
    assert(current != last);

    if (current + 2 > last || current[0] != '0' || state.current != current + 1)
        return false;

    assert(current[1] == 'x' || current[1] == 'X');
    current += 2;

    assert(current != last);

    return parse_integer_digits(state, current, last, 16U, out);
}

CLASSIFY_SCALAR_FORCE_INLINE bool parse_floating(
    const parse_state& state,
    double* out) noexcept {
    const char* first = state.numeric_first;
    const char* last = state.last;
    const std::size_t size = static_cast<std::size_t>(last - first);
    assert(first != last);
    if (size > 4096)
        return false;

#if CLASSIFY_SCALAR_HAS_STD_FLOAT_FROM_CHARS
    double parsed = 0;
    const std::from_chars_result result = std::from_chars(first, last, parsed);
    if (result.ec != std::errc() || result.ptr != last || !std::isfinite(parsed))
        return false;

    if (out)
        *out = parsed;

    return true;
#else
    char buffer[4097];
    std::size_t i = 0;
    for (const char* current = first; current != last; ++current, ++i) {
        const unsigned char c = static_cast<unsigned char>(*current);
        if (is_ascii_space(static_cast<char>(c)))
            return false;

        buffer[i] = static_cast<char>(c);
    }
    buffer[size] = '\0';

    char* parse_end = nullptr;
    errno = 0;
    const long double parsed = std::strtold(buffer, &parse_end);
    if (parse_end != buffer + size || errno == ERANGE || !std::isfinite(parsed))
        return false;

    if (out)
        *out = static_cast<double>(parsed);

    return true;
#endif
}

CLASSIFY_SCALAR_FORCE_INLINE bool floating_is_integral(const double value, std::int64_t* out) noexcept {
    if (value < int64_min_long_double || value > int64_max_long_double)
        return false;

    const std::int64_t integer = static_cast<std::int64_t>(value);
    if (static_cast<double>(integer) != value)
        return false;

    if (out)
        *out = integer;

    return true;
}

CLASSIFY_SCALAR_FORCE_INLINE ScalarKind finish_integer(
    const std::int64_t,
    classify_only_output&) noexcept {
    return scalar_int;
}

template<typename Output>
CLASSIFY_SCALAR_FORCE_INLINE ScalarKind finish_integer(
    const std::int64_t parsed_integer,
    Output& output) noexcept {
    output.set_integer(parsed_integer);
    return scalar_int;
}

CLASSIFY_SCALAR_FORCE_INLINE ScalarKind finish_floating(
    const double parsed_float,
    classify_only_output&) noexcept {
    if (floating_is_integral(parsed_float, nullptr))
        return scalar_int;

    return scalar_float;
}

template<typename Output>
CLASSIFY_SCALAR_FORCE_INLINE ScalarKind finish_floating(
    const double parsed_float,
    Output& output) noexcept {
    std::int64_t parsed_integer = 0;
    if (floating_is_integral(parsed_float, &parsed_integer))
        return finish_integer(parsed_integer, output);

    output.set_number(parsed_float);
    return scalar_float;
}

template<bool RecognizeHex = true>
struct builtin_numeric_policy {
    CLASSIFY_SCALAR_CONST CONSTEXPR_14 static bool matches_leading(unsigned char c) noexcept {
        return ascii_digit_value(c) || c == '.' || c == '+' || c == '-';
    }

    template<typename Output>
    CLASSIFY_SCALAR_FORCE_INLINE ScalarKind on_dispatch(
        parse_state& state,
        Output& output) const noexcept {
        return on_number(state, output);
    }

    template<typename Output>
    CLASSIFY_SCALAR_FORCE_INLINE ScalarKind on_decimal(
        parse_state& state,
        Output& output) const noexcept {
        double parsed_float = 0;
        return parse_floating(state, &parsed_float)
            ? finish_floating(parsed_float, output)
            : scalar_string;
    }

    template<typename Output>
    CLASSIFY_SCALAR_FORCE_INLINE ScalarKind on_exponent(
        parse_state& state,
        Output& output) const noexcept {
        if (state.current == state.first || state.current + 1 == state.last)
            return scalar_string;

        double parsed_float = 0;
        return parse_floating(state, &parsed_float)
            ? finish_floating(parsed_float, output)
            : scalar_string;
    }

    template<typename Output>
    CLASSIFY_SCALAR_FORCE_INLINE ScalarKind on_hex_prefix(
        parse_state& state,
        Output& output) const noexcept {
        if (state.current == state.first || state.current + 1 == state.last)
            return scalar_string;

        std::int64_t parsed_integer = 0;
        return parse_hex_integer(state, &parsed_integer)
            ? finish_integer(parsed_integer, output)
            : scalar_string;
    }

    template<typename Output>
    CLASSIFY_SCALAR_FORCE_INLINE ScalarKind on_end(
        parse_state& state,
        Output& output) const noexcept {
        std::int64_t parsed_integer = 0;
        return parse_decimal_integer(state, &parsed_integer)
            ? finish_integer(parsed_integer, output)
            : scalar_string;
    }

    template<typename Output>
    CLASSIFY_SCALAR_FORCE_INLINE ScalarKind scan_number(
        parse_state& state,
        const char* scan_first,
        Output& output) const noexcept {
        for (const char* current = scan_first; current != state.last; ++current) {
            state.current = current;
            const unsigned char c = static_cast<unsigned char>(*current);
            if (is_ascii_digit(c))
                continue;

            const ParseFlag flag = parse_table<RecognizeHex>()[c];
            switch (flag) {
            case ParseFlag::decimal:
                return on_decimal(state, output);
            case ParseFlag::might_be_exponential:
                return on_exponent(state, output);
            case ParseFlag::might_be_hex_prefix:
                return on_hex_prefix(state, output);
            case ParseFlag::other:
            default:
                return scalar_string;
            }
        }

        state.current = state.last;
        return on_end(state, output);
    }

    template<typename Output>
    CLASSIFY_SCALAR_FORCE_INLINE ScalarKind on_number(
        parse_state& state,
        Output& output) const noexcept {
        const unsigned char first_char = static_cast<unsigned char>(*state.first);
        const char* value_first = state.first;

        if (first_char == '+' || first_char == '-') {
            state.sign = first_char == '-'
                ? parse_state::negative_sign
                : parse_state::positive_sign;
            state.numeric_first = state.sign == parse_state::negative_sign
                ? state.first
                : state.first + 1;
            value_first = state.first + 1;
            if (value_first == state.last)
                return scalar_string;
        }

        const unsigned char value_first_char = static_cast<unsigned char>(*value_first);
        if (value_first_char == '.') {
            state.current = value_first;
            return on_decimal(state, output);
        }

        if (!is_ascii_digit(value_first_char))
            return scalar_string;

        return scan_number(state, value_first + 1, output);
    }
};

struct builtin_bool_policy {
    CLASSIFY_SCALAR_CONST CONSTEXPR_14 static bool matches_leading(unsigned char c) noexcept {
        return c == 't' || c == 'T' || c == 'f' || c == 'F';
    }

    template<typename Output>
    CLASSIFY_SCALAR_FORCE_INLINE ScalarKind on_dispatch(
        parse_state& state,
        Output& output) const noexcept {
        bool parsed = false;
        if (!(parse_true(state.first, state.last, &parsed) || parse_false(state.first, state.last, &parsed)))
            return scalar_string;

        output.set_bool(parsed);
        return scalar_bool;
    }
};

struct builtin_timestamp_policy {
    CLASSIFY_SCALAR_CONST CONSTEXPR_14 static bool matches_leading(unsigned char c) noexcept {
        return ascii_digit_value(c);
    }

    template<typename Output>
    CLASSIFY_SCALAR_FORCE_INLINE ScalarKind on_dispatch(
        parse_state& state,
        Output&) const noexcept {
        return parse_iso_timestamp(state.first, state.last)
            ? scalar_timestamp
            : scalar_string;
    }
};

template<typename PolicyPack, typename Output>
CLASSIFY_SCALAR_FORCE_INLINE ScalarKind classify_leading_dispatch(
    const char* first,
    const char* last,
    Output& output,
    const PolicyPack& policies) noexcept {
    parse_state state(first, last);
    const unsigned char policy_index = dispatch_table<PolicyPack>()[static_cast<unsigned char>(*first)];
    return policies.dispatch(policy_index, state, output);
}

} // namespace detail

typedef detail::parse_state parse_state;

template<typename... Policies>
using policy_pack = detail::policy_pack<Policies...>;

template<bool RecognizeHex = true>
using builtin_numeric_policy = detail::builtin_numeric_policy<RecognizeHex>;

typedef detail::builtin_bool_policy builtin_bool_policy;

typedef detail::builtin_timestamp_policy builtin_timestamp_policy;

typedef detail::policy_pack<
    detail::builtin_timestamp_policy,
    detail::builtin_numeric_policy<true>,
    detail::builtin_bool_policy> builtin_policy_pack;

typedef builtin_policy_pack default_policy_pack;

typedef detail::policy_pack<
    detail::builtin_numeric_policy<true>,
    detail::builtin_bool_policy> numeric_bool_policy_pack;

typedef detail::policy_pack<
    detail::builtin_numeric_policy<true> > numeric_policy_pack;

namespace detail {

template<
    typename Output,
    typename PolicyPack>
CLASSIFY_SCALAR_FORCE_INLINE ScalarKind classify_scalar_trimmed(
    const char* first,
    const char* last,
    Output& output,
    const PolicyPack& policies) noexcept {
    return first == last ? scalar_null :
        classify_leading_dispatch(first, last, output, policies);
}

} // namespace detail

template<
    bool TrimAsciiWhitespace = true,
    typename Output = classify_only_output,
    typename Policy = default_policy_pack>
CLASSIFY_SCALAR_FORCE_INLINE ScalarKind classify_scalar(
    const char* first,
    const char* last,
    Output output = Output(),
    Policy policy = Policy()) noexcept {
    if (!first || !last || last < first)
        return scalar_string;

    const scalar_span span = TrimAsciiWhitespace
        ? detail::trim_ascii(first, last)
        : scalar_span{first, last};

    return detail::classify_scalar_trimmed(
        span.first,
        span.last,
        output,
        policy);
}

CLASSIFY_SCALAR_FORCE_INLINE ScalarKind classify_scalar(
    const char* first,
    const char* last,
    classify_only_output output = classify_only_output()) noexcept {
    return classify_scalar<true>(first, last, output, default_policy_pack());
}

template<
    bool TrimAsciiWhitespace = true,
    std::size_t Size,
    typename Output = classify_only_output,
    typename Policy = default_policy_pack>
CLASSIFY_SCALAR_FORCE_INLINE ScalarKind classify_scalar(
    const char (&value)[Size],
    Output output = Output(),
    Policy policy = Policy()) noexcept {
    return classify_scalar<TrimAsciiWhitespace>(
        value,
        value + Size - 1,
        output,
        policy);
}

#ifdef CLASSIFY_SCALAR_HAS_CXX17
template<
    bool TrimAsciiWhitespace = true,
    typename Output = classify_only_output,
    typename Policy = default_policy_pack>
CLASSIFY_SCALAR_FORCE_INLINE ScalarKind classify_scalar(
    std::string_view value,
    Output output = Output(),
    Policy policy = Policy()) noexcept {
    if (value.empty())
        return scalar_null;

    return classify_scalar<TrimAsciiWhitespace>(
        value.data(),
        value.data() + value.size(),
        output,
        policy);
}

CLASSIFY_SCALAR_FORCE_INLINE ScalarKind classify_scalar(
    std::string_view value,
    classify_only_output output = classify_only_output()) noexcept {
    return classify_scalar<true>(value, output, default_policy_pack());
}

#endif

} // namespace classify_scalar
