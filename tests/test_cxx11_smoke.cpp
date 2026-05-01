#include <classify_scalar.hpp>

#include <cstddef>
#include <cstdint>

template<typename Output>
classify_scalar::ScalarKind classify_literal(
    const char* value,
    const std::size_t size,
    Output output) {
    return classify_scalar::classify_scalar(value, value + size, output);
}

template<typename Output, typename Policy>
classify_scalar::ScalarKind classify_literal(
    const char* value,
    const std::size_t size,
    Output output,
    Policy policy) {
    return classify_scalar::classify_scalar(value, value + size, output, policy);
}

int main() {
    std::int64_t integer = 0;
    long double number = 0;
    bool boolean = false;

    classify_scalar::builtin_output_refs outputs = classify_scalar::output_refs(number, integer, boolean);

    if (classify_literal("42", 2, outputs) != classify_scalar::scalar_int) {
        return 1;
    }
    if (integer != 42) {
        return 2;
    }
    const char* false_value = "false";
    if (classify_scalar::classify_scalar(false_value, false_value + 5, outputs) != classify_scalar::scalar_bool) {
        return 3;
    }
    if (boolean) {
        return 4;
    }
    if (classify_literal("1e-3", 4, outputs) != classify_scalar::scalar_float) {
        return 5;
    }
    if (classify_literal("9223372036854775808", 19, classify_scalar::classify_only_output()) != classify_scalar::scalar_bigint) {
        return 6;
    }
    if (classify_literal(
        "true",
        4,
        classify_scalar::classify_only_output(),
        classify_scalar::numeric_policy_pack()) != classify_scalar::scalar_string) {
        return 7;
    }
    typedef classify_scalar::policy_pack<
        classify_scalar::builtin_numeric_policy<','> > comma_numeric_policy_pack;
    if (classify_literal(
        "3,14",
        4,
        classify_scalar::classify_only_output(),
        comma_numeric_policy_pack()) != classify_scalar::scalar_float) {
        return 8;
    }

    return 0;
}
