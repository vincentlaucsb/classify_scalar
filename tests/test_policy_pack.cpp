#include <catch2/catch_test_macros.hpp>

#include <classify_scalar.hpp>

namespace {

enum class app_scalar_kind : int {
    CLASSIFY_SCALAR_BUILTINS,
    money
};

struct money_policy {
    static constexpr bool matches_leading(unsigned char c) noexcept {
        return c == '$';
    }

    template<typename Output>
    app_scalar_kind on_dispatch(
        classify_scalar::parse_state& state,
        Output&) const noexcept {
        return state.last - state.first == 3
                && state.first[0] == '$'
                && state.first[1] == '4'
                && state.first[2] == '2'
            ? app_scalar_kind::money
            : app_scalar_kind::scalar_string;
    }
};

} // namespace

TEST_CASE("policy packs can add custom leading-byte classifiers") {
    typedef classify_scalar::policy_pack<
        money_policy,
        classify_scalar::builtin_numeric_policy<>,
        classify_scalar::builtin_bool_policy> pack_type;

    const pack_type pack;

    CHECK(static_cast<int>(classify_scalar::classify_scalar(
        "$42",
        classify_scalar::classify_only_output(),
        pack)) == static_cast<int>(app_scalar_kind::money));

    CHECK(classify_scalar::classify_scalar<app_scalar_kind>(
        "$42",
        classify_scalar::classify_only_output(),
        pack) == app_scalar_kind::money);

    CHECK(classify_scalar::classify_scalar<app_scalar_kind>(
        "$43",
        classify_scalar::classify_only_output(),
        pack) == app_scalar_kind::scalar_string);

    CHECK(classify_scalar::classify_scalar<app_scalar_kind>(
        "42",
        classify_scalar::classify_only_output(),
        pack) == app_scalar_kind::scalar_int8);

    CHECK(classify_scalar::classify_scalar<app_scalar_kind>(
        "true",
        classify_scalar::classify_only_output(),
        pack) == app_scalar_kind::scalar_bool);
}

TEST_CASE("policy packs only dispatch policies they contain") {
    typedef classify_scalar::policy_pack<money_policy> pack_type;

    CHECK(classify_scalar::classify_scalar<app_scalar_kind>(
        "42",
        classify_scalar::classify_only_output(),
        pack_type()) == app_scalar_kind::scalar_string);
}
