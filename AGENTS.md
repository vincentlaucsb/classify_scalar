# classify_scalar Agent Guide

This repository is a small, header-only scalar classification library split out
from csv-parser/csvzall scalar inference work.

## Project Shape

- Keep the library single-header only.
- The public header is `include/classify_scalar.hpp`.
- Do not add library `.cpp` files. Tests may use `.cpp` files.
- Keep the public header C++11 compatible. Newer standards may get nicer
  overloads or aliases when available, but the baseline must compile as C++11.
- C++20-only conveniences may use concepts to improve diagnostics for custom
  policy packs, but they must be guarded so C++11/C++17 users do not see them.
- Use the bundled parser for built-in integer conversion in all language modes.
  Prefer `std::from_chars` for floating-point conversion when compiling as C++17
  or newer and the standard library provides a real implementation. Keep C++11
  fallback parsers in the header for older callers.
- Keep hot-path parser options compile-time first. `TrimAsciiWhitespace` is the
  public classifier template knob. Built-in recognizers should be selected by
  composing policy packs, such as omitting `builtin_bool_policy`. The built-in
  numeric policy recognizes `0x` hexadecimal integers and does not recognize
  bare hex by default. Its first template argument selects the decimal
  separator, such as `builtin_numeric_policy<','>`, and its second template
  argument controls whether integral-valued floating syntax returns a signed
  integer-width scalar kind. Keep runtime dispatchers tiny and explicit, such as the provided
  dot/comma numeric decimal-symbol helper.
- Use prefixed local compatibility macros (`CLASSIFY_SCALAR_CONSTEXPR_14`,
  `CLASSIFY_SCALAR_CONSTEXPR_VALUE_14`,
  `CLASSIFY_SCALAR_CONSTEXPR_17`,
  `CLASSIFY_SCALAR_CONSTEXPR_VALUE_17`, and `CLASSIFY_SCALAR_CONST`) for cross-standard
  constexpr and compiler attribute concerns.
- Keep the API close to C with templates: pointer spans, plain structs, integer
  kind ids, and small free functions.
- Prefer pointer spans (`const char* first`, `const char* last`) for hot-path
  APIs and implementation helpers. C++17 may expose thin `std::string_view`
  convenience overloads at the public boundary only. Never store view types in
  persistent data structures.
- Treat classify_scalar as a runtime classifier first. Use compile-time
  machinery where it pays for hot-path behavior, such as policy composition,
  ASCII/dispatch tables, and C++20 diagnostics. Avoid compile-time bloat for
  convenience-only APIs, clever type plumbing, or overloads that users can
  trivially write themselves around the pointer-span core.
- No German engineering: do not turn a small API request into a framework,
  switchboard, policy annex, or second parser. Prefer the boring local change
  that solves the stated problem.
- Avoid tiny internal helper functions that are only called once, especially in
  the public header. A helper should usually earn its existence by being reused,
  isolating genuinely tricky behavior, improving testability, or keeping a
  hot-path function readable. If the helper body is shorter than its signature
  and has one caller, inline it unless there is a clear semantic reason not to.
- The public header must be self-contained for compiler diagnostics. Fix public
  header compile errors and warnings directly, or suppress deliberate
  compatibility warnings with tightly scoped, prefixed macros/pragmas inside
  `include/classify_scalar.hpp`.
- Suppressing compiler warnings through target/compiler flags is only acceptable
  for tests, benchmarks, or local harnesses. Do not make downstream users carry
  warning suppressions for this header.

## Scalar Inference Contract

- Classify well-formed scalar literals after optional ASCII-boundary trimming.
- Do not repair arbitrary internal whitespace or normalize malformed domain
  strings.
- Keep conservative inference separate from explicit parsing helpers.
  `classify_scalar` should avoid surprising guesses such as bare hex, while
  `parse_scalar<kind>` may accept a specific grammar directly.
- Boundary trimming is enabled by default. Internal parser helpers should receive
  the trimmed pointer span.
- Keep the common numeric path cheap. Classification without storage should use
  the default no-op output policy. Built-in storage should use
  `output_refs(number, integer, boolean)`, and future custom policies may define
  their own output objects with matching hooks. Prefer showing extension by
  deriving from `builtin_output_refs` and adding domain-specific setters, so
  built-in numeric/bool storage keeps working.
- Return exact signed integer-width scalar ids (`scalar_int8`, `scalar_int16`,
  `scalar_int32`, or `scalar_int64`). Reserve unsigned ids for a future unsigned
  policy, but do not classify to them until that policy exists. Decimal integers
  outside int64 classify as `scalar_bigint`; do not allocate or store the full
  bigint value on the built-in path.
  Custom policies should define their own enum values with
  `CLASSIFY_SCALAR_BUILTINS` and can return that enum directly when callers use
  the typed classifier template.
- Optimize for the common path while preserving custom type extensibility.
- Use compile-time ASCII tables for parser-family dispatch. Built-in dispatch
  maps leading bytes to numeric or bool policy families. Numeric internals own
  digit, decimal marker, sign, exponent marker, and hex-prefix handling.
- User extension should go through ordered policy packs. Each policy declares
  `matches_leading(unsigned char)` and handles matching spans with
  `on_dispatch(parse_state&, output&)`; earlier policies have higher priority.
  If a policy returns `scalar_string`, dispatch falls through to the next policy
  whose leading-byte matcher accepts the same input.
- Prefer a switch-driven scan loop over accumulating scattered per-character
  `if` statements.
- Put look-ahead and look-behind decisions in small policy handlers that receive
  a shared parse-state reference with raw pointer context (`first`, `last`,
  `current`) so future custom recognizers can inspect local byte spans without
  changing the core scanner. Do not allocate a context object per scanned byte.
- Keep scalar-specific helper calls inside the relevant policy handler when
  possible, including boolean recognizers (`t/T` and `f/F`) as well as numeric
  markers. Avoid a separate discovery pass followed by a second parse pass for
  the common built-in kinds.
- Prefer compile-time-aware parse policy types over defensive runtime checks in
  the switch. Built-in policy packs own which recognizers are present, and each
  interesting `ParseFlag` has an explicit handler.
- Keep parse and dispatch tables policy-aware. If bool recognition is disabled
  at compile time, bool starter bytes should not dispatch to the bool policy.
  The numeric parse table should map the active decimal separator to
  `ParseFlag::decimal` and always map `x`/`X` to hex-prefix handling.
- The bool parser currently uses a SWAR-style 32-bit word load plus lowercase
  bitmask for `true`/`false`. Benchmarks have been noisy: narrow losses with
  occasional larger wins. Treat this as a deliberate experiment and re-test it
  against the simpler ASCII lowercase-table implementation when touching bool
  parsing or code layout.
- Keep public wrappers responsible for pointer validation, output validation,
  and optional trimming. The internal trimmed classifier handles the empty
  trimmed-span/null decision before calling the hot numeric switch.

## Initial Built-ins

- string
- bool
- float
- int, including `0x` hexadecimal and scientific notation when the final value is
  an integer
- bigint for decimal integer literals outside int64
- conservative ISO date/date-time timestamp strings, such as `YYYY-MM-DD` and
  `YYYY-MM-DDTHH:MM:SSZ`

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
- Prefer no braces for simple one-line `if` bodies, especially guard returns
  and direct assignments. Keep braces when an `else`, nesting, or future edit
  risk would make the control flow harder to read.
- Avoid unrelated refactors or metadata churn.
