# classify_scalar

High-performance, header-only scalar classification for C++11 and newer.

## Motivation
I was creating a general purpose ETL tool for CSV files and needed a fast and reliable way to determine whether a field was a number, bool, basic string, an ISO 8601 timestamp, etc.

I had a similar piece of code in my CSV parser library, but with each new feature add, it had turned into a `switch` soup.

I wanted something that was just as fast (or even faster), and wanted it to be extensible.

## Built-In Examples

```cpp
#include <classify_scalar/classify_scalar.hpp>

using namespace classify_scalar;

auto text = classify_scalar("hello");                 // scalar_string
auto empty = classify_scalar("");                     // scalar_null
auto whitespace = classify_scalar("   ");             // scalar_null

auto yes = classify_scalar("true");                   // scalar_bool
auto no = classify_scalar("FALSE");                   // scalar_bool

auto integer_kind = classify_scalar("42");            // scalar_int
auto negative_kind = classify_scalar("-17");          // scalar_int
auto hex_kind = classify_scalar("0x2a");              // scalar_int

auto float_kind = classify_scalar("3.14159");         // scalar_float
auto integral_exp = classify_scalar("-1.25e2");       // scalar_int
auto fractional_exp = classify_scalar("1e-3");        // scalar_float

auto date = classify_scalar("2024-01-31");            // scalar_timestamp
auto datetime = classify_scalar("2024-01-31T23:59:58Z"); // scalar_timestamp
```

ASCII boundary whitespace is trimmed by default:

```cpp
auto trimmed = classify_scalar::classify_scalar("  42  ");       // scalar_int
auto exact = classify_scalar::classify_scalar<false>("  42  ");  // scalar_string
```

Calling `classify_scalar(...)` without an output policy means classify only.
Use `output_refs(number, integer, boolean)` when you want built-in parsed values
stored:

```cpp
std::int64_t integer = 0;
long double number = 0;
bool boolean = false;

auto kind = classify_scalar::classify_scalar(
    "  -0x2a  ",
    classify_scalar::output_refs(number, integer, boolean));

// kind == scalar_int
// integer == -42
// number == -42.0L
```

## Design

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
Future custom policies can provide their own output object with matching
`set_*` hooks.

Integer conversion uses the bundled parser in all language modes. When compiled
as C++17 or newer, floating-point conversion uses `std::from_chars` where the
standard library provides a real implementation; older builds use the bundled
fallback parser.

## Policy Packs

Hot-path behavior is selected at compile time. `TrimAsciiWhitespace` is the
public classifier template knob; scalar families are selected by policy pack.

```cpp
using no_bool_pack = classify_scalar::policy_pack<
    classify_scalar::builtin_timestamp_policy,
    classify_scalar::builtin_numeric_policy<true>>;

auto no_bools = classify_scalar::classify_scalar(
    "true",
    classify_scalar::classify_only_output(),
    no_bool_pack());
```

Policies are ordinary types. A policy provides:

- `matches_leading(unsigned char)`, used to build the compile-time dispatch
  table.
- `on_dispatch(parse_state&, output&)`, which returns a `ScalarKind`.

See `tests/test_telephone_policy.cpp` for a complete custom NANP telephone
number recognizer that returns a user-defined scalar kind while falling through
to the built-in numeric, bool, and timestamp policies.

## Standards

The public header remains C++11 compatible. The core API and implementation use
`const char*` pointer spans. C++17 builds also provide thin `std::string_view`
overloads. C++20 builds add concepts to improve diagnostics for malformed
custom policy packs.
