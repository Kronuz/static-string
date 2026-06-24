# static-string

A header-only, constexpr compile-time string for C++17.

## What it is

`static_string` builds and concatenates fixed strings entirely at compile time.
You write `static_string::string("Hello, ") + static_string::string("world") +
'!'` and the result, including its length, is a constant computed by the
compiler. It converts implicitly to `const char*` and `std::string_view` (and
explicitly to `std::string`), so a value you assemble at compile time drops
straight into ordinary runtime code.

## When to use it / when not

Use it when you want to assemble fixed strings without paying for it at runtime:
table headers, ANSI color escape sequences, compile-time hashing seeds, labels
whose pieces are known when you compile. Everything happens in `constexpr`
context with no heap allocation and no runtime string work, and the conversions
let the finished value flow into normal APIs.

Skip it if your strings are runtime data. The length is part of the type
(`static_string<13, char_array>` is a different type from
`static_string<14, char_array>`), so anything you concatenate has to be known to
the compiler. Long concatenations also grow template instantiations, which costs
compile time rather than runtime. For dynamic strings, use `std::string`.

## Install

Header-only. Copy `static_string.hh` somewhere on your include path and include
it:

```cpp
#include "static_string.hh"
```

With CMake there's an `INTERFACE` target (`static_string`) you can link against.
To pull it in via `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(
  static_string
  GIT_REPOSITORY https://github.com/Kronuz/static-string.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(static_string)

target_link_libraries(your_target PRIVATE static_string)
```

The `static_string` target adds the include directory and requests `cxx_std_17`.

There is no third-party dependency. When compiled as C++20 (with `<format>`), an
optional `std::formatter` specialization is provided so `static_string` values
work directly in `std::format(...)`; define `STATIC_STRING_NO_FORMAT` to turn it
off. It is only a convenience, since `static_string` already converts to
`std::string_view`, which `std::format` formats directly.

## Usage

All examples assume `#include "static_string.hh"` and a `namespace ss =
static_string;` alias.

### Compile-time concatenation

```cpp
namespace ss = static_string;

// Literals and a single char, concatenated at compile time.
constexpr auto greeting = ss::string("Hello, ") + ss::string("world") + '!';
static_assert(greeting.size() == 13, "size");
// greeting.c_str() == "Hello, world!"

// You can mix in raw string literals on either side of +.
constexpr auto path = ss::string("/usr") + "/local";
// std::string_view(path) == "/usr/local"
```

### Integers and chars to strings

```cpp
// Integer -> string, computed by the compiler.
constexpr auto n = ss::to_string<2025>();
// n.c_str() == "2025"

// A single char as a one-element string.
constexpr auto x = ss::char_to_string<'x'>();
// x.c_str() == "x"
```

### The string(...) factory

```cpp
// string() wraps a raw literal and deduces its length (no null terminator).
constexpr auto s = ss::string("abc");   // static_string<3, literal_ref>
static_assert(s.size() == 3, "size");

// A single char goes through the owning char_array flavor.
constexpr auto c = ss::string('z');     // static_string<1, char_array>
```

### Comparisons

```cpp
// == and < are constexpr and compare across both flavors.
static_assert(ss::string("abc") == ss::string("abc"), "eq");
static_assert(!(ss::string("abc") == ss::string("abd")), "neq");
static_assert(ss::string("abc") < ss::string("abd"), "lt");
```

### Conversions

```cpp
constexpr auto label = ss::string("count=") + ss::to_string<42>();

const char* cstr = label;                  // implicit operator const char*
std::string_view sv = label;               // implicit operator string_view
std::string owned = std::string(label);    // explicit operator std::string

// label.c_str(), label.data(), label.size(), label.length() also work.
```

## API reference

Everything lives in `namespace static_string` in `static_string.hh`.

### The value type

```cpp
template <std::size_t N, typename T>
class static_string;
```

`N` is the string length **without** the null terminator. `T` is a tag that
picks one of two flavors (the unspecialized primary template is a hard error,
`static_string.hh:60`):

- `string_literal_ref<N>` is `static_string<N, literal_ref>`
  (`static_string.hh:66`). It is a non-owning wrapper over a string literal
  reference, `const char (&)[N + 1]` (`static_string.hh:78`). No copy, no
  storage of its own. Produced by `string("literal")`.
- `string_char_array<N>` is `static_string<N, char_array>`
  (`static_string.hh:70`). It owns a stack-stored, null-terminated `char[N + 1]`
  (`static_string.hh:155`). Every concatenation result is a `char_array`.

A `string_literal_ref<N>` converts to `string_char_array<N>` by copying its
characters into the owning buffer (`static_string.hh:176`).

### Factories

```cpp
template <std::size_t N_PLUS_1>
constexpr auto string(const char (&s)[N_PLUS_1]);   // -> string_literal_ref<N_PLUS_1 - 1>
constexpr auto string(const char ch);               // -> string_char_array<1>
```

`string(...)` wraps a raw string literal as a non-owning `string_literal_ref`
and deduces the length, dropping the null terminator (`static_string.hh:221`).
The `char` overload produces a one-element owning string (`static_string.hh:227`).

```cpp
template <int num>  constexpr auto to_string();        // -> string_char_array<digits>
template <char ch>  constexpr auto char_to_string();   // -> string_char_array<1>
```

`to_string<int>()` renders a non-negative integer to its decimal digits at
compile time (`static_string.hh:137`); `to_string<0>()` is `"0"`.
`char_to_string<char>()` makes a one-element owning string from a char
(`static_string.hh:144`).

### Concatenation operators

```cpp
operator+(static_string, static_string)   // -> string_char_array<NL + NR>
operator+(const char (&)[...], static_string)
operator+(static_string, const char (&)[...])
operator+(char, static_string)
operator+(static_string, char)
```

All five overloads (`static_string.hh:246`-`269`) return an owning
`string_char_array` whose size is the sum of the operand sizes, computed at
compile time. Operands may be either flavor, raw literals, or single chars in
any combination.

Two extra `operator+` overloads (`static_string.hh:271`, `:279`) take a
`static_string` on one side and an arbitrary other type on the other and return
a runtime `std::string`. These are the escape hatch for mixing a compile-time
string with a runtime one; they allocate and are not `constexpr`.

### Accessors and conversions

Both flavors expose the same interface (`static_string.hh:84`-`113` and
`:185`-`215`):

```cpp
constexpr std::size_t size()   const;   // == N
constexpr std::size_t length() const;   // == N
constexpr const char* c_str()  const;   // null-terminated
constexpr const char* data()   const;
constexpr char        operator[](std::size_t i) const;

template <typename OT, std::size_t ON>
constexpr bool operator==(const static_string<ON, OT>&) const;
template <typename OT, std::size_t ON>
constexpr bool operator<(const static_string<ON, OT>&) const;

constexpr operator const char* () const;        // implicit
constexpr operator std::string_view() const;    // implicit
explicit operator std::string() const;          // explicit
```

`==` and `<` are templated over the other operand's `N` and tag, so they compare
across both flavors. `==` short-circuits on a length mismatch; `<` is plain
lexicographic with the shorter string ordering first on a common prefix.

## Build & test

Header-only, so there's nothing to compile for use. To run the smoke test:

```sh
c++ -std=c++17 -I. test/test.cc -o test/test && ./test/test
# or with CMake:
cmake -B build && cmake --build build && ctest --test-dir build
```

The test (`test/test.cc`) checks compile-time concatenation, `to_string<2025>()`,
raw-literal mixing, and `constexpr` comparison. It uses `assert`, so build
without `NDEBUG`. Requires a C++17 compiler.

## Notes & caveats

- The length is baked into the type. `static_string<3, ...>` and
  `static_string<4, ...>` are distinct types, so you generally store results in
  `auto` (ideally `constexpr auto`) rather than naming the type.
- `string("literal")` is a non-owning `string_literal_ref`: it holds a reference
  to the literal's storage and copies nothing. The result of any `+`, by
  contrast, is an owning `string_char_array` with its own stack buffer.
- `to_string<int>()` handles non-negative integers; there is no minus-sign
  handling.
- Long concatenation chains grow template instantiations and slow the compiler.
  This is a compile-time cost, not a runtime one.
- The `constexpr_assert` checks (bounds, null termination, size match) fire as
  runtime `assert`s outside constant evaluation and are compiled out under
  `NDEBUG` (`static_string.hh:53`).
- The optional `std::formatter` (C++20) is pure convenience; the implicit
  `std::string_view` conversion already lets `std::format` print a
  `static_string`.

## Provenance

Extracted from [Xapiand](https://github.com/Kronuz/Xapiand), where it is vendored
and underpins `ansi_color.hh` (the compile-time colors) and the compile-time
hashing. The `literal_ref` core derives from Andrzej Krzemienski's `constexpr`
string work and is substantially extended here.

## License

Boost Software License 1.0. Copyright (c) 2015-2019 Dubalu LLC and Copyright (c)
2017 Andrzej Krzemienski. See [LICENSE](LICENSE).
