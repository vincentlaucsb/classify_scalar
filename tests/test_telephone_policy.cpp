#include <catch2/catch_test_macros.hpp>

#include <classify_scalar/classify_scalar.hpp>

#include <cstddef>

// This file is a live extension example, not a built-in telephone parser.
//
// The core library dispatches to policy types selected at compile time. A
// policy only needs two pieces:
//   1. matches_leading(byte): should this policy get first chance for a span?
//   2. on_dispatch(state, output): classify the already-trimmed pointer span.
//
// `telephone_pack` puts `telephone_policy` before the built-ins, so NANP-style
// phone numbers return a custom ScalarKind while ordinary numbers and timestamps
// fall through to the stock policies. This keeps the header small while showing
// how users can add domain-specific scalar recognizers in C++11.

namespace {

enum class app_scalar_kind : int {
    telephone = classify_scalar::scalar_custom_begin
};

constexpr classify_scalar::ScalarKind scalar_telephone =
    classify_scalar::to_scalar_kind(app_scalar_kind::telephone);

bool is_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}

bool is_separator(char c) noexcept {
    return c == ' ' || c == '-' || c == '.';
}

bool is_nanp_digits(const char* digits, std::size_t offset) noexcept {
    return digits[offset] >= '2'
        && digits[offset + 3] >= '2';
}

bool parse_nanp_telephone(const char* first, const char* last) noexcept {
    char digits[11] = {};
    std::size_t count = 0;
    bool open_area = false;
    bool closed_area = false;

    for (const char* current = first; current != last; ++current) {
        const char c = *current;
        if (is_digit(c)) {
            if (count == 11)
                return false;

            digits[count++] = c;
            continue;
        }

        if (c == '+') {
            if (current != first)
                return false;

            continue;
        }

        if (c == '(') {
            if (open_area || closed_area || !(count == 0 || (count == 1 && digits[0] == '1')))
                return false;

            open_area = true;
            continue;
        }

        if (c == ')') {
            const std::size_t expected_count = digits[0] == '1' ? 4 : 3;
            if (!open_area || closed_area || count != expected_count)
                return false;

            closed_area = true;
            continue;
        }

        if (is_separator(c))
            continue;

        return false;
    }

    if (open_area && !closed_area)
        return false;

    if (count == 10)
        return is_nanp_digits(digits, 0);

    return count == 11 && digits[0] == '1' && is_nanp_digits(digits, 1);
}

struct telephone_policy {
    static constexpr bool matches_leading(unsigned char c) noexcept {
        return c == '+'
            || c == '('
            || (c >= '0' && c <= '9');
    }

    template<typename Output>
    classify_scalar::ScalarKind on_dispatch(
        classify_scalar::parse_state& state,
        Output&) const noexcept {
        return parse_nanp_telephone(state.first, state.last)
            ? classify_scalar::to_scalar_kind(app_scalar_kind::telephone)
            : classify_scalar::scalar_string;
    }
};

typedef classify_scalar::policy_pack<
    telephone_policy,
    classify_scalar::builtin_timestamp_policy,
    classify_scalar::builtin_numeric_policy<>,
    classify_scalar::builtin_bool_policy> telephone_pack;

} // namespace

TEST_CASE("custom telephone policy classifies NANP numbers without a country prefix") {
    CHECK(classify_scalar::classify_scalar(
        "2125551212",
        classify_scalar::classify_only_output(),
        telephone_pack()) == scalar_telephone);

    CHECK(classify_scalar::classify_scalar(
        "212-555-1212",
        classify_scalar::classify_only_output(),
        telephone_pack()) == scalar_telephone);

    CHECK(classify_scalar::classify_scalar(
        "(212) 555-1212",
        classify_scalar::classify_only_output(),
        telephone_pack()) == scalar_telephone);

    CHECK(classify_scalar::classify_scalar(
        "212.555.1212",
        classify_scalar::classify_only_output(),
        telephone_pack()) == scalar_telephone);
}

TEST_CASE("custom telephone policy classifies NANP numbers with a country prefix") {
    CHECK(classify_scalar::classify_scalar(
        "+1 212 555 1212",
        classify_scalar::classify_only_output(),
        telephone_pack()) == scalar_telephone);

    CHECK(classify_scalar::classify_scalar(
        "+1-212-555-1212",
        classify_scalar::classify_only_output(),
        telephone_pack()) == scalar_telephone);

    CHECK(classify_scalar::classify_scalar(
        "+1 (212) 555-1212",
        classify_scalar::classify_only_output(),
        telephone_pack()) == scalar_telephone);
}

TEST_CASE("custom telephone policy rejects malformed or non-NANP numbers") {
    CHECK(classify_scalar::classify_scalar(
        "112-555-1212",
        classify_scalar::classify_only_output(),
        telephone_pack()) == classify_scalar::scalar_string);

    CHECK(classify_scalar::classify_scalar(
        "212-155-1212",
        classify_scalar::classify_only_output(),
        telephone_pack()) == classify_scalar::scalar_string);

    CHECK(classify_scalar::classify_scalar(
        "+44 20 7946 0958",
        classify_scalar::classify_only_output(),
        telephone_pack()) == classify_scalar::scalar_string);

    CHECK(classify_scalar::classify_scalar(
        "(212 555-1212",
        classify_scalar::classify_only_output(),
        telephone_pack()) == classify_scalar::scalar_string);
}

TEST_CASE("custom telephone policy falls through to built-ins") {
    CHECK(classify_scalar::classify_scalar(
        "42",
        classify_scalar::classify_only_output(),
        telephone_pack()) == classify_scalar::scalar_int);

    CHECK(classify_scalar::classify_scalar(
        "2024-01-31",
        classify_scalar::classify_only_output(),
        telephone_pack()) == classify_scalar::scalar_timestamp);
}
