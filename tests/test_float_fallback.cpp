#include <catch2/catch_test_macros.hpp>

#define CLASSIFY_SCALAR_DISABLE_STD_FLOAT_FROM_CHARS
#include <classify_scalar.hpp>

#include <cstdint>
#include <cstring>

#if (defined(_MSVC_LANG) ? _MSVC_LANG : __cplusplus) >= 201703L && !defined(_LIBCPP_VERSION)
#include <charconv>
#include <system_error>
#define CLASSIFY_SCALAR_TEST_HAS_FLOAT_FROM_CHARS_REFERENCE
#endif

using classify_scalar::scalar_bigfloat;
using classify_scalar::scalar_float;

namespace {
double reference_parse_float(const char* literal) {
#ifdef CLASSIFY_SCALAR_TEST_HAS_FLOAT_FROM_CHARS_REFERENCE
    double parsed = 0;
    const char* last = literal + std::strlen(literal);
    const std::from_chars_result result = std::from_chars(literal, last, parsed);
    REQUIRE(result.ec == std::errc());
    REQUIRE(result.ptr == last);
    return parsed;
#else
    if (std::strcmp(literal, "438.6344381717412") == 0)
        return 438.6344381717412;
    if (std::strcmp(literal, "128.4258295010055") == 0)
        return 128.4258295010055;
    if (std::strcmp(literal, "13.01158360912943") == 0)
        return 13.01158360912943;
    if (std::strcmp(literal, "8.2362477") == 0)
        return 8.2362477;
    if (std::strcmp(literal, "45.7060766") == 0)
        return 45.7060766;
    if (std::strcmp(literal, "45.7113177") == 0)
        return 45.7113177;
    if (std::strcmp(literal, "-82.236045") == 0)
        return -82.236045;
    if (std::strcmp(literal, "-113.011629") == 0)
        return -113.011629;

    FAIL("missing fallback float reference literal");
    return 0;
#endif
}
} // namespace

TEST_CASE("fallback floating parser matches reference conversion for dense decimal data") {
    typedef classify_scalar::policy_pack<
        classify_scalar::builtin_numeric_policy<'.', false> > floating_syntax_pack;

    const char* literals[] = {
        "438.6344381717412",
        "128.4258295010055",
        "13.01158360912943",
        "8.2362477",
        "45.7060766",
        "45.7113177",
        "-82.236045",
        "-113.011629"
    };

    for (const char* literal : literals) {
        CAPTURE(literal);

        const double expected = reference_parse_float(literal);

        double parsed = 0;
        REQUIRE(classify_scalar::parse_float(literal, literal + std::strlen(literal), parsed));
        CHECK(parsed == expected);

        std::int64_t integer = 0;
        long double number = 0;
        bool boolean = false;
        CHECK(classify_scalar::classify_scalar(
            literal,
            literal + std::strlen(literal),
            classify_scalar::output_refs(number, integer, boolean),
            floating_syntax_pack()) == scalar_float);
        CHECK(static_cast<double>(number) == expected);
    }
}

TEST_CASE("fallback floating parser reports big floats outside finite double range") {
    typedef classify_scalar::policy_pack<
        classify_scalar::builtin_numeric_policy<'.', false> > floating_syntax_pack;

    const char* literals[] = {
        "1e309",
        "-1e309"
    };

    for (const char* literal : literals) {
        CAPTURE(literal);

        double parsed = 0;
        REQUIRE_FALSE(classify_scalar::parse_float(literal, literal + std::strlen(literal), parsed));

        std::int64_t integer = 0;
        long double number = 0;
        bool boolean = false;
        CHECK(classify_scalar::classify_scalar(
            literal,
            literal + std::strlen(literal),
            classify_scalar::output_refs(number, integer, boolean),
            floating_syntax_pack()) == scalar_bigfloat);
    }
}
