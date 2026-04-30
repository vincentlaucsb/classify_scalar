#include <catch2/catch_test_macros.hpp>

#include <classify_scalar/classify_scalar.hpp>

namespace {

enum class app_scalar_kind : int {
    money = classify_scalar::scalar_custom_begin
};

struct money_policy {
    static constexpr bool matches_leading(unsigned char c) noexcept {
        return c == '$';
    }

    template<typename Output>
    classify_scalar::ScalarKind on_dispatch(
        classify_scalar::parse_state& state,
        Output&) const noexcept {
        return state.last - state.first == 3
                && state.first[0] == '$'
                && state.first[1] == '4'
                && state.first[2] == '2'
            ? classify_scalar::to_scalar_kind(app_scalar_kind::money)
            : classify_scalar::scalar_string;
    }
};

} // namespace

TEST_CASE("policy packs can add custom leading-byte classifiers") {
    typedef classify_scalar::policy_pack<
        money_policy,
        classify_scalar::builtin_numeric_policy<>,
        classify_scalar::builtin_bool_policy> pack_type;

    const pack_type pack;

    CHECK(classify_scalar::classify_scalar(
        "$42",
        classify_scalar::classify_only_output(),
        pack) == classify_scalar::to_scalar_kind(app_scalar_kind::money));

    CHECK(classify_scalar::classify_scalar(
        "$43",
        classify_scalar::classify_only_output(),
        pack) == classify_scalar::scalar_string);

    CHECK(classify_scalar::classify_scalar(
        "42",
        classify_scalar::classify_only_output(),
        pack) == classify_scalar::scalar_int);

    CHECK(classify_scalar::classify_scalar(
        "true",
        classify_scalar::classify_only_output(),
        pack) == classify_scalar::scalar_bool);
}

TEST_CASE("policy packs only dispatch policies they contain") {
    typedef classify_scalar::policy_pack<money_policy> pack_type;

    CHECK(classify_scalar::classify_scalar(
        "42",
        classify_scalar::classify_only_output(),
        pack_type()) == classify_scalar::scalar_string);
}
