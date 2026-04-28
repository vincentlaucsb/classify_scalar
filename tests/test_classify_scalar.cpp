#include <catch2/catch_test_macros.hpp>

#include <classify_scalar/classify_scalar.hpp>

#include <string_view>

using classify_scalar::scalar_int;
using classify_scalar::scalar_null;
using classify_scalar::scalar_string;

TEST_CASE("public convenience overloads classify expected spans") {
    CHECK(classify_scalar::classify_scalar("") == scalar_null);
    CHECK(classify_scalar::classify_scalar(std::string_view{}) == scalar_null);
    CHECK(classify_scalar::classify_scalar(std::string_view{"42"}) == scalar_int);
    CHECK(classify_scalar::classify_scalar<false>("42") == scalar_int);
    CHECK(classify_scalar::classify_scalar(static_cast<const char*>(nullptr), static_cast<const char*>(nullptr)) == scalar_string);
}
