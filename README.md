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
integers, and exponential notation. ASCII boundary whitespace is trimmed by
default.

Calling `classify_scalar(...)` without an output policy means classify only.
Use `output_refs(number, integer, boolean)` when you want the built-in parsed
values stored. Future custom policies can provide their own output object with
matching `set_*` hooks.

The classifier uses a compile-time ASCII `ParseFlag` table and a switch-driven
loop. Each interesting flag dispatches to a policy handler, and the built-in
policy calls the specific parse helper for decimal, exponent, hex-prefix, true,
and false cases. Custom policies receive a mutable parse state with raw pointer
context (`first`, `last`, `current`) and scanner facts such as the first sign.

When compiled as C++17 or newer, numeric conversion uses `std::from_chars`.
C++11 builds use the bundled fallback parsers.

Hot-path behavior can be selected at compile time:

```cpp
auto exact = classify_scalar::classify_scalar<false>("  42  ");
auto no_bools = classify_scalar::classify_scalar<true, false>("true");
```

C++17 builds also provide thin `std::string_view` overloads. The core API and
implementation use `const char*` pointer spans.
