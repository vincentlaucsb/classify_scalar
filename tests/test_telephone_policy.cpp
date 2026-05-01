#include <catch2/catch_test_macros.hpp>

#include <classify_scalar.hpp>

#include <cstddef>
#include <cstdint>

// This file is a live extension example, not a built-in telephone parser.
//
// The core library dispatches to policy types selected at compile time. A
// policy only needs two pieces:
//   1. matches_leading(byte): should this policy get first chance for a span?
//   2. on_dispatch(state, output): classify the already-trimmed pointer span.
//
// `telephone_pack` puts `telephone_policy` before the built-ins, so NANP-style
// phone numbers return a custom enum value while ordinary numbers and timestamps
// fall through to the stock policies. `telephone_output_refs` also shows how a
// user can extend the built-in output refs with a domain-specific setter while
// keeping the built-in numeric/bool setters. This keeps the header small while
// showing how users can add domain-specific scalar recognizers in C++11.

namespace {

enum class MyTypes : int {
    CLASSIFY_SCALAR_BUILTINS,
    telephone
};

constexpr MyTypes scalar_telephone = MyTypes::telephone;

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

bool parse_nanp_telephone(const char* first, const char* last, std::uint64_t* out = nullptr) noexcept {
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

    std::size_t offset = 0;
    if (count == 10) {
        if (!is_nanp_digits(digits, 0))
            return false;
    } else if (count == 11 && digits[0] == '1' && is_nanp_digits(digits, 1)) {
        offset = 1;
    } else {
        return false;
    }

    if (out) {
        std::uint64_t parsed = 1;
        for (std::size_t i = offset; i < offset + 10; ++i)
            parsed = (parsed * 10) + static_cast<unsigned>(digits[i] - '0');

        *out = parsed;
    }

    return true;
}

struct telephone_output_refs : classify_scalar::builtin_output_refs {
    telephone_output_refs(
        long double& number_,
        std::int64_t& integer_,
        bool& boolean_,
        std::uint64_t& telephone_) noexcept
        : classify_scalar::builtin_output_refs(number_, integer_, boolean_),
          telephone(telephone_) {}

    void set_telephone(std::uint64_t value) const noexcept {
        telephone = value;
    }

    std::uint64_t& telephone;
};

struct telephone_classify_only_output : classify_scalar::classify_only_output {
    void set_telephone(std::uint64_t) const noexcept {}
};

struct telephone_policy {
    static constexpr bool matches_leading(unsigned char c) noexcept {
        return c == '+'
            || c == '('
            || (c >= '0' && c <= '9');
    }

    template<typename Output>
    MyTypes on_dispatch(
        classify_scalar::parse_state& state,
        Output& output) const noexcept {
        std::uint64_t parsed = 0;
        if (!parse_nanp_telephone(state.first, state.last, &parsed))
            return MyTypes::scalar_string;

        output.set_telephone(parsed);
        return MyTypes::telephone;
    }
};

typedef classify_scalar::policy_pack<
    telephone_policy,
    classify_scalar::builtin_numeric_policy<>,
    classify_scalar::builtin_timestamp_policy,
    classify_scalar::builtin_bool_policy> telephone_pack;

} // namespace

TEST_CASE("custom telephone policy classifies NANP numbers without a country prefix") {
    CHECK(classify_scalar::classify_scalar<MyTypes>(
        "2125551212",
        telephone_classify_only_output(),
        telephone_pack()) == scalar_telephone);

    CHECK(classify_scalar::classify_scalar<MyTypes>(
        "212-555-1212",
        telephone_classify_only_output(),
        telephone_pack()) == scalar_telephone);

    CHECK(classify_scalar::classify_scalar<MyTypes>(
        "(212) 555-1212",
        telephone_classify_only_output(),
        telephone_pack()) == scalar_telephone);

    CHECK(classify_scalar::classify_scalar<MyTypes>(
        "212.555.1212",
        telephone_classify_only_output(),
        telephone_pack()) == scalar_telephone);
}

TEST_CASE("custom telephone policy classifies NANP numbers with a country prefix") {
    CHECK(classify_scalar::classify_scalar<MyTypes>(
        "+1 212 555 1212",
        telephone_classify_only_output(),
        telephone_pack()) == scalar_telephone);

    CHECK(classify_scalar::classify_scalar<MyTypes>(
        "+1-212-555-1212",
        telephone_classify_only_output(),
        telephone_pack()) == scalar_telephone);

    CHECK(classify_scalar::classify_scalar<MyTypes>(
        "+1 (212) 555-1212",
        telephone_classify_only_output(),
        telephone_pack()) == scalar_telephone);
}

TEST_CASE("custom telephone policy rejects malformed or non-NANP numbers") {
    CHECK(classify_scalar::classify_scalar<MyTypes>(
        "112-555-1212",
        telephone_classify_only_output(),
        telephone_pack()) == MyTypes::scalar_string);

    CHECK(classify_scalar::classify_scalar<MyTypes>(
        "212-155-1212",
        telephone_classify_only_output(),
        telephone_pack()) == MyTypes::scalar_string);

    CHECK(classify_scalar::classify_scalar<MyTypes>(
        "+44 20 7946 0958",
        telephone_classify_only_output(),
        telephone_pack()) == MyTypes::scalar_string);

    CHECK(classify_scalar::classify_scalar<MyTypes>(
        "(212 555-1212",
        telephone_classify_only_output(),
        telephone_pack()) == MyTypes::scalar_string);
}

TEST_CASE("custom telephone policy falls through to built-ins") {
    CHECK(classify_scalar::classify_scalar<MyTypes>(
        "42",
        telephone_classify_only_output(),
        telephone_pack()) == MyTypes::scalar_int);

    CHECK(classify_scalar::classify_scalar<MyTypes>(
        "2024-01-31",
        telephone_classify_only_output(),
        telephone_pack()) == MyTypes::scalar_timestamp);
}

TEST_CASE("custom telephone policy can extend output refs") {
    long double number = 0;
    std::int64_t integer = 0;
    bool boolean = false;
    std::uint64_t telephone = 0;

    telephone_output_refs outputs(number, integer, boolean, telephone);

    CHECK(classify_scalar::classify_scalar<MyTypes>(
        "(212) 555-1212",
        outputs,
        telephone_pack()) == scalar_telephone);
    CHECK(telephone == 12125551212ULL);

    CHECK(classify_scalar::classify_scalar<MyTypes>(
        "+1 646 555 0100",
        outputs,
        telephone_pack()) == scalar_telephone);
    CHECK(telephone == 16465550100ULL);

    CHECK(classify_scalar::classify_scalar<MyTypes>(
        "42",
        outputs,
        telephone_pack()) == MyTypes::scalar_int);
    CHECK(integer == 42);
    CHECK(number == 42.0L);
}
