# classify_scalar Agent Guide

This repository is a small, header-only scalar classification library split out
from csv-parser/csvzall scalar inference work.

## Project Shape

- Keep the library single-header only.
- The public header is `include/classify_scalar/classify_scalar.hpp`.
- Do not add library `.cpp` files. Tests may use `.cpp` files.
- Keep the public header C++11 compatible. Newer standards may get nicer
  overloads or aliases when available, but the baseline must compile as C++11.
- Prefer `std::from_chars` for built-in numeric conversion when compiling as
  C++17 or newer. Keep C++11 fallback parsers in the header for older callers.
- Keep hot-path parser options compile-time first. `TrimAsciiWhitespace`,
  `RecognizeBool`, and `RecognizeHex` should be template parameters on the
  primary classifier so the compiler can erase unused branches. Avoid runtime
  option-dispatch overloads unless a concrete downstream need justifies the
  compile-time cost.
- Use the local compatibility macros copied from csv-parser (`IF_CONSTEXPR`,
  `CONSTEXPR`, `CONSTEXPR_VALUE`, `CONSTEXPR_14`, `CONSTEXPR_VALUE_14`,
  `CONSTEXPR_17`, and `CLASSIFY_SCALAR_CONST`) for cross-standard constexpr
  and compiler attribute concerns.
- Keep the API close to C with templates: pointer spans, plain structs, integer
  kind ids, and small free functions.
- Prefer pointer spans (`const char* first`, `const char* last`) for hot-path
  APIs and implementation helpers. C++17 may expose thin `std::string_view`
  convenience overloads at the public boundary only. Never store view types in
  persistent data structures.

## Scalar Inference Contract

- Classify well-formed scalar literals after optional ASCII-boundary trimming.
- Do not repair arbitrary internal whitespace or normalize malformed domain
  strings.
- Boundary trimming is enabled by default. Internal parser helpers should receive
  the trimmed pointer span.
- Keep the common numeric path cheap. Classification without storage should use
  the default no-op output policy. Built-in storage should use
  `output_refs(number, integer, boolean)`, and future custom policies may define
  their own output objects with matching hooks.
- Return integer kind ids. The built-in enum names reserve the default ids, and
  future custom policies should use ids starting at the documented custom range.
- Optimize for the common path while preserving custom type extensibility.
- Use a compile-time ASCII parse table that maps characters to `ParseFlag`
  dispatch hints such as digit, decimal marker, exponent marker, and hex-prefix
  marker.
- Prefer a switch-driven scan loop over accumulating scattered per-character
  `if` statements.
- Put look-ahead and look-behind decisions in small policy handlers that receive
  a shared parse-state reference with raw pointer context (`first`, `last`,
  `current`) so future custom recognizers can inspect local byte spans without
  changing the core scanner. Do not allocate a context object per scanned byte.
- Keep scalar-specific helper calls inside the relevant `ParseFlag` policy
  handler when possible, including boolean recognizers (`t/T` and `f/F`) as
  well as numeric markers. Avoid a separate discovery pass followed by a second
  parse pass for the common built-in kinds.
- Prefer compile-time-aware parse policy types over defensive runtime checks in
  the switch. The built-in policy owns `RecognizeBool`/`RecognizeHex` and each
  interesting `ParseFlag` has an explicit handler.
- Keep the parse table policy-aware for table-driven flags. Bool starter bytes
  and numeric signs are handled before the numeric switch because they are only
  significant at the first byte. If hex recognition is disabled at compile time,
  the table should not map bytes to hex `ParseFlag` values.
- Keep public wrappers responsible for pointer validation, output validation,
  and optional trimming. The internal trimmed classifier handles the empty
  trimmed-span/null decision before calling the hot numeric switch.

## Initial Built-ins

- string
- bool
- float
- int, including hexadecimal and scientific notation when the final value is an
  integer

Whitespace-only input is treated as null so callers can share csv-parser-style
empty-field semantics.

## CMake And Tests

- Use CMake and Catch2 for tests.
- Prefer `find_package(Catch2 3 CONFIG QUIET)` first.
- It is acceptable for CMake to fetch Catch2 when no local/system package is
  available.
- Keep tests focused and behavior-driven. Add regression tests for numeric
  boundaries when changing integer parsing.

## Formatting

- Prefer LF (`\n`) line endings for tracked source, tests, CMake, and Markdown.
- Keep preprocessor directives flush left.
- Avoid unrelated refactors or metadata churn.
