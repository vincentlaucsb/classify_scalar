#include <classify_scalar/classify_scalar.hpp>

namespace {

struct concept_policy {
    static constexpr bool matches_leading(unsigned char c) noexcept {
        return c == '#';
    }

    template<typename Output>
    classify_scalar::ScalarKind on_dispatch(
        classify_scalar::parse_state&,
        Output&) const noexcept {
        return classify_scalar::scalar_custom_begin;
    }
};

} // namespace

int main() {
    classify_scalar::policy_pack<concept_policy> pack;
    return classify_scalar::classify_scalar("#", classify_scalar::classify_only_output(), pack)
        == classify_scalar::scalar_custom_begin
        ? 0
        : 1;
}
