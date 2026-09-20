#include <classify_scalar.hpp>
#include <cmath>

#if defined(EXPECT_FLOAT_FROM_CHARS) && EXPECT_FLOAT_FROM_CHARS
#ifndef CLASSIFY_SCALAR_HAS_STD_FLOAT_FROM_CHARS
#error "The positive capability probe was not honored"
#endif
#elif defined(EXPECT_FLOAT_FROM_CHARS)
#ifdef CLASSIFY_SCALAR_HAS_STD_FLOAT_FROM_CHARS
#error "The negative probe or explicit opt-out was not honored"
#endif
#endif

int main() {
    double value = 0;
    const char text[] = "-1.25e2";
    return !classify_scalar::parse_float(text, text + sizeof(text) - 1, value)
        || !std::isfinite(value) || value != -125.0;
}
