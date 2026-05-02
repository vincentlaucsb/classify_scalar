#define CONSTEXPR_14 host_constexpr_14
#define CONSTEXPR_VALUE_14 host_constexpr_value_14
#define CONSTEXPR_17 host_constexpr_17
#define CONSTEXPR_VALUE_17 host_constexpr_value_17

#include <classify_scalar.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

#define CLASSIFY_SCALAR_TEST_STRINGIFY_INNER(value) #value
#define CLASSIFY_SCALAR_TEST_STRINGIFY(value) CLASSIFY_SCALAR_TEST_STRINGIFY_INNER(value)

static_assert(CLASSIFY_SCALAR_VERSION_MAJOR == 1, "unexpected classify_scalar major version");
static_assert(CLASSIFY_SCALAR_VERSION_MINOR == 0, "unexpected classify_scalar minor version");
static_assert(CLASSIFY_SCALAR_VERSION_PATCH == 0, "unexpected classify_scalar patch version");
static_assert(CLASSIFY_SCALAR_VERSION == 10000, "unexpected classify_scalar version number");

TEST_CASE("classify_scalar compatibility macros do not claim generic host names") {
    CHECK(CLASSIFY_SCALAR_TEST_STRINGIFY(CONSTEXPR_14) == std::string("host_constexpr_14"));
    CHECK(CLASSIFY_SCALAR_TEST_STRINGIFY(CONSTEXPR_VALUE_14) == std::string("host_constexpr_value_14"));
    CHECK(CLASSIFY_SCALAR_TEST_STRINGIFY(CONSTEXPR_17) == std::string("host_constexpr_17"));
    CHECK(CLASSIFY_SCALAR_TEST_STRINGIFY(CONSTEXPR_VALUE_17) == std::string("host_constexpr_value_17"));
    CHECK(classify_scalar::classify_scalar("42") == classify_scalar::scalar_int8);
}
