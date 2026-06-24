# AGENTS

Notes for agents and contributors working in this repo.

## Repo map

- `static_string.hh` — the entire library. One class template,
  `static_string<N, T>`, specialized two ways (`literal_ref`, `char_array`),
  plus the `string(...)` factory, `to_string<>` / `char_to_string<>`, the `+`
  operators, and an optional `std::formatter` (C++20). Header-only.
- `test/test.cc` — smoke test exercising compile-time concatenation,
  `to_string<2025>()`, raw-literal mixing, and `constexpr` comparison.
- `CMakeLists.txt` — defines the `static_string` `INTERFACE` target and a
  `ctest` test (built only when this repo is top-level).
- `LICENSE` — Boost Software License 1.0; dual copyright (Dubalu LLC and
  Andrzej Krzemienski).
- `README.md` — usage and API reference.
- `ARCHITECTURE.md` — internal design (the two flavors, integer_sequence
  concatenation, the `explode` template, `constexpr_assert`).

## Build & run the test

```sh
c++ -std=c++17 -I. test/test.cc -o test/test && ./test/test
# or:
cmake -B build && cmake --build build && ctest --test-dir build
```

The test prints `static-string OK: ...` and exits 0 on success. It uses
`assert` and `static_assert`, so build without `NDEBUG` to keep the runtime
checks live.

## Conventions

- C++17. The design leans on `std::integer_sequence`, `std::make_integer_sequence`,
  variadic templates, and `__has_include`.
- Header-only. Keep everything in `static_string.hh`; there is no `.cc` to
  compile for the library itself.
- `constexpr`-heavy: constructors, operators, factories, and comparisons are all
  meant to work in constant evaluation. Anything new should too, unless it is
  deliberately a runtime escape hatch (like the `std::string` `operator+`
  overloads at `static_string.hh:271`).
- Indentation is tabs, matching the existing source.
- Use double quotes in code blocks per the docs' style.

## Invariants

- In `static_string<N, T>`, `N` is the length **without** the null terminator.
  The backing array is `N + 1` (`static_string.hh:78`, `:155`). Off-by-one here
  is the easiest mistake.
- `string("literal")` produces `string_literal_ref<N_PLUS_1 - 1>`: it drops the
  literal's null terminator from `N` (`static_string.hh:221`).
- `literal_ref` is non-owning; it holds a reference to the literal's storage and
  copies nothing. `char_array` is owning, with its own stack buffer. Every `+`
  result is a `char_array`.
- For the `+` operators the sizes and types must line up. The two-operand
  constructor splits a result of size `N` at `M` and expects a left operand of
  size `M` and a right of size `N - M` (`static_string.hh:159`,`:172`). The
  operator overloads compute the result size as the sum of operand sizes; a
  mismatch is a compile error, not a silent bug.
- Constructors assert their preconditions via `constexpr_assert`: the literal is
  null-terminated at `N`, a `char` constructor gets `N == 1`, the
  `(const char*, size)` constructor gets `N == size`. In constant evaluation a
  violation fails the compile; at runtime it is an `assert`, gone under `NDEBUG`.

## How to extend

- New compile-time operations should produce a `string_char_array<N>` and build
  it through one of the existing private constructors, sizing `N` from
  compile-time constants so the result stays `constexpr`.
- Adding an `operator+` combination means adding an overload that wraps each
  operand into a `static_string` (a `string_literal_ref` for raw literals, a
  `string_char_array<1>` for chars) and forwards to the two-operand constructor
  with the summed size, mirroring `static_string.hh:251`-`269`.
- Keep the accessor and comparison surface in sync across **both**
  specializations (`static_string.hh:84` and `:185`); they are intentionally
  duplicated.
- Add a matching `assert` / `static_assert` block to `test/test.cc` for any new
  behavior, and prefer `static_assert` so the check runs at compile time.

## Traps

- `N` excludes the null terminator but the buffer is `N + 1`. Mixing the two up
  corrupts sizes and termination.
- `to_string<int>()` is non-negative only; there is no minus-sign path
  (`static_string.hh:124`). `explode<0, 0>` exists specifically so `0` renders
  as `"0"` and not the empty string.
- The runtime `std::string` `operator+` overloads (`static_string.hh:271`,
  `:279`) silently catch any non-`static_string` operand. If a compile-time
  concatenation unexpectedly allocates, an operand probably didn't match the
  `constexpr` overloads and fell through to these.
- The `digits` of `explode` accumulate by **prepending** to the char pack
  (`static_string.hh:124`); appending would reverse the number.
- The `std::formatter` is gated on C++20 + `__has_include(<format>)` and off
  under `STATIC_STRING_NO_FORMAT`. It is a convenience only; `static_string`
  already converts to `std::string_view`.

## Provenance

This code was extracted from
[Xapiand](https://github.com/Kronuz/Xapiand), where it is vendored and underpins
`ansi_color.hh` (the compile-time colors) and the compile-time hashing. The
`literal_ref` core derives from Andrzej Krzemienski's `constexpr` string work.
Keep the Boost Software License header and the dual copyright intact when editing
`static_string.hh`.
