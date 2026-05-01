# classify_scalar

![classify_scalar logo](assets/classify_scalar_logo.png)

High-performance, header-only scalar classification for C++11 and newer.

## Motivation
I was creating a general purpose ETL tool for CSV files and needed a fast and reliable way to determine whether a field was a number, bool, basic string, an ISO 8601 timestamp, etc.

I experimented with other methods, but I wanted something that was easily extensible to support arbitrary user-provided types.

## Built-In Examples

The library supports parsing booleans (`true` or `false` of varying casings), ISO 8601 timestamps, floats, and ints out of the box.

```cpp
#include <classify_scalar.hpp>

using namespace classify_scalar;

auto text = classify_scalar("hello");                 // scalar_string
auto empty = classify_scalar("");                     // scalar_null
auto whitespace = classify_scalar("   ");             // scalar_null

auto yes = classify_scalar("true");                   // scalar_bool
auto no = classify_scalar("FALSE");                   // scalar_bool

auto integer_kind = classify_scalar("42");            // scalar_int
auto negative_kind = classify_scalar("-17");          // scalar_int
auto hex_kind = classify_scalar("0x2a");              // scalar_int
auto too_large = classify_scalar("9223372036854775808"); // scalar_bigint

auto float_kind = classify_scalar("3.14159");         // scalar_float
auto integral_exp = classify_scalar("-1.25e2");       // scalar_int
auto fractional_exp = classify_scalar("1e-3");        // scalar_float

auto date = classify_scalar("2024-01-31");            // scalar_timestamp
auto datetime = classify_scalar("2024-01-31T23:59:58Z"); // scalar_timestamp
```

ASCII boundary whitespace is trimmed by default:

```cpp
auto trimmed = classify_scalar::classify_scalar("  42  ");       // scalar_int
auto exact = classify_scalar::classify_scalar<ScalarKind, false>("  42  ");  // scalar_string
```

Calling `classify_scalar(...)` without an output policy means classify only.
Use `output_refs(number, integer, boolean)` when you want built-in parsed values
stored:

```cpp
std::int64_t integer = 0;
long double number = 0;
bool boolean = false;
IntegerKind integer_kind = integer_none;

auto kind = classify_scalar::classify_scalar(
    "  -0x2a  ",
    classify_scalar::output_refs(number, integer, boolean, integer_kind));

// kind == scalar_int
// integer == -42
// number == -42.0L
// integer_kind == integer_int8
```

## Explicit Parsing

Use `classify_scalar(...)` when you want conservative inference. Use
`parse_scalar<kind>(...)` for built-in kinds when you already know which grammar
you want:

```cpp
std::int64_t hex = 0;
bool ok = classify_scalar::parse_hex("DEADBEEF", hex);
// ok == true
// hex == 0xDEADBEEF
// classify_scalar("DEADBEEF") would still be scalar_string

double number = 0;
const char* float_first = "1e-3";
classify_scalar::parse_scalar<classify_scalar::scalar_float>(
    float_first,
    float_first + 4,
    number);

std::uint64_t timestamp = 0;
const char* timestamp_first = "2024-01-31T23:59:58Z";
classify_scalar::parse_scalar<classify_scalar::scalar_timestamp>(
    timestamp_first,
    timestamp_first + 20,
    timestamp);
```

`parse_scalar<scalar_int>` reuses the normal numeric classifier. Use `parse_hex`
when bare hexadecimal should be accepted explicitly without making inference
classify `DEADBEEF` as an integer. The parser supports optional
ASCII-boundary trimming through its second template argument.
`parse_scalar<scalar_timestamp>` returns a JavaScript-style Unix timestamp in
milliseconds, normalized to UTC, and currently requires a non-negative result
because the natural home is `std::uint64_t`. `scalar_bigint` remains
classification-only because the built-in path deliberately avoids allocating or
storing the full integer.

## Extending the Classifier/Parser

To add a custom scalar type, define:

- an enum value for the new kind
- a policy that recognizes and parses it
- an output object *if you want to store parsed values*

Policies are ordinary types. A policy provides:

- `matches_leading(unsigned char)`, used to build the compile-time dispatch
  table.
- `on_dispatch(parse_state&, output&)`, which returns a `ScalarKind`.

Custom policies can use an application enum for their own scalar kinds:

```cpp
enum class app_scalar_kind : int {
    CLASSIFY_SCALAR_BUILTINS,
    telephone
};

struct telephone_policy {
    static constexpr bool matches_leading(unsigned char c) noexcept {
        return c == '+' || c == '(' || (c >= '0' && c <= '9');
    }

    template<typename Output>
    app_scalar_kind on_dispatch(classify_scalar::parse_state& state, Output& out) const noexcept {
        // Parse the field and call out.set_telephone(...) here.
        return app_scalar_kind::telephone;
    }
};

auto kind = classify_scalar::classify_scalar<app_scalar_kind>(
    "(212) 555-1212",
    outputs,
    classify_scalar::policy_pack<
        telephone_policy,
        classify_scalar::builtin_timestamp_policy,
        classify_scalar::builtin_numeric_policy<>,
        classify_scalar::builtin_bool_policy>());
```

`CLASSIFY_SCALAR_BUILTINS` copies the library's built-in kind ids into your enum
and positions the next enum value at `scalar_custom_begin`. The typed
`classify_scalar<app_scalar_kind>(...)` overload returns your enum directly, so
built-ins return values such as `app_scalar_kind::scalar_int` and custom
policies can return values such as `app_scalar_kind::telephone`.

Custom policies can provide their own output object with matching `set_*` hooks.
The simple path is to derive from `builtin_output_refs` and add domain setters:

```cpp
struct app_outputs : classify_scalar::builtin_output_refs {
    app_outputs(long double& n, std::int64_t& i, bool& b, std::uint64_t& phone)
        : builtin_output_refs(n, i, b), telephone(phone) {}

    void set_telephone(std::uint64_t value) const noexcept { telephone = value; }

    std::uint64_t& telephone;
};
```

See `tests/test_telephone_policy.cpp` for a complete custom NANP telephone
number recognizer that returns a user-defined scalar kind while falling through
to the built-in numeric, bool, and timestamp policies.

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

Integer conversion uses the bundled parser in all language modes. When compiled
as C++17 or newer, floating-point conversion uses `std::from_chars` where the
standard library provides a real implementation; older builds use the bundled
fallback parser.

Decimal integer literals outside int64 classify as `scalar_bigint` without
allocating or storing the full integer text. Hexadecimal inference is limited to
`0x`/`0X` prefixes by default; bare hex strings such as `FF` remain strings.

## Policy Packs

Hot-path behavior is selected at compile time. `TrimAsciiWhitespace` is the
public classifier template knob; scalar families are selected by policy pack.

```cpp
using no_bool_pack = classify_scalar::policy_pack<
    classify_scalar::builtin_timestamp_policy,
    classify_scalar::builtin_numeric_policy<>>;

auto no_bools = classify_scalar::classify_scalar(
    "true",
    classify_scalar::classify_only_output(),
    no_bool_pack());
```

For numeric-only inference, pass `numeric_policy_pack`:

```cpp
auto kind = classify_scalar::classify_scalar(
    "2024-01-31",
    classify_scalar::classify_only_output(),
    classify_scalar::numeric_policy_pack()); // scalar_string
```

The built-in numeric policy recognizes hexadecimal integers by default. Its
template argument selects the decimal separator:

```cpp
using comma_decimal_pack = classify_scalar::policy_pack<
    classify_scalar::builtin_numeric_policy<','>,
    classify_scalar::builtin_bool_policy>;

auto value = classify_scalar::classify_scalar("3,14",
    classify_scalar::classify_only_output(),
    comma_decimal_pack()); // scalar_float
```

For runtime decimal symbols, keep the dispatch at your integration boundary:

```cpp
switch (decimal_symbol) {
case '.':
    return classify_scalar::classify_scalar(first, last);
case ',':
    return classify_scalar::classify_scalar(
        first,
        last,
        classify_scalar::classify_only_output(),
        comma_decimal_pack());
default:
    return classify_scalar::scalar_invalid;
}
```

The default numeric policy returns `scalar_int` for integral-valued floating
syntax such as `1e3` and `-1.25e2`. If an integration needs scientific or decimal
syntax to stay `scalar_float`, use the second numeric policy template argument:

```cpp
using floating_syntax_pack = classify_scalar::policy_pack<
    classify_scalar::builtin_numeric_policy<'.', false>>;
```

## Standards

 * The public header remains C++11 compatible. The core API and implementation use
`const char*` pointer spans.
 * C++17 builds also provide thin `std::string_view`
overloads.
 * C++20 builds add concepts to improve diagnostics for malformed
custom policy packs.

We use `std::from_chars` for floating point parsing where available, but fall back to a hand-rolled verison otherwise.

The header defines `CLASSIFY_SCALAR_VERSION_MAJOR`,
`CLASSIFY_SCALAR_VERSION_MINOR`, `CLASSIFY_SCALAR_VERSION_PATCH`, and numeric
`CLASSIFY_SCALAR_VERSION` macros. If multiple vendored copies are included in
one translation unit, an older copy included after a newer copy is skipped; a
newer copy included after an older copy fails loudly because the older
definitions have already been emitted.
