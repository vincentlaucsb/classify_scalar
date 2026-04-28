#include <classify_scalar/classify_scalar.hpp>

#include <cstdint>

int main() {
    std::int64_t integer = 0;
    long double number = 0;
    bool boolean = false;

    classify_scalar::scalar_outputs outputs(&number, &integer, &boolean);

    if (classify_scalar::classify_scalar("42", outputs) != classify_scalar::scalar_int) {
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
    if (classify_scalar::classify_scalar("1e-3", outputs) != classify_scalar::scalar_float) {
        return 5;
    }

    return 0;
}
