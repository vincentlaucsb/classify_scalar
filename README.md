# classify_scalar

High-performance header-only scalar classification for C++11 and newer.

```cpp
#include <classify_scalar/classify_scalar.hpp>

std::int64_t integer = 0;
long double number = 0;
bool boolean = false;

auto kind = classify_scalar::classify_scalar(
    "  -0x2a  ",
    classify_scalar::output_refs(number, integer, boolean));
```

The initial API classifies strings, booleans, integers, floats, hexadecimal
integers, exponential notation, and conservative ISO date/date-time timestamps.
ASCII boundary whitespace is trimmed by default.

Calling `classify_scalar(...)` without an output policy means classify only.
Use `output_refs(number, integer, boolean)` when you want the built-in parsed
values stored. Future custom policies can provide their own output object with
matching `set_*` hooks.

The classifier uses compile-time ASCII tables for leading-byte dispatch. The
top-level classifier selects a parser family such as timestamp, bool, or
numeric, and the built-in numeric policy owns the decimal, exponent, and
hex-prefix scan.
Custom policies receive a mutable parse state with raw pointer context
(`first`, `last`, `current`) and scanner facts such as the first sign.
User policy packs are ordered by priority: the first policy whose
`matches_leading(unsigned char)` returns true receives the trimmed span through
`on_dispatch(parse_state&, output&)`. If that policy returns `scalar_string`,
the pack falls through to the next policy that matches the same leading byte.

Integer conversion uses the bundled parser in all language modes. When compiled
as C++17 or newer, floating-point conversion uses `std::from_chars` where the
standard library provides a real implementation; older builds use the bundled
fallback parser.

Hot-path behavior can be selected at compile time:

```cpp
auto exact = classify_scalar::classify_scalar<false>("  42  ");

using no_bool_pack = classify_scalar::policy_pack<
    classify_scalar::builtin_timestamp_policy,
    classify_scalar::builtin_numeric_policy<true>>;

auto no_bools = classify_scalar::classify_scalar(
    "true",
    classify_scalar::classify_only_output(),
    no_bool_pack());
```

C++17 builds also provide thin `std::string_view` overloads. The core API and
implementation use `const char*` pointer spans.
