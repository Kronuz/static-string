# static-string

Part of [Kronuz C++ Libraries](https://github.com/Kronuz/cpp-libraries), a collection of small, reusable C++ libraries.

[![CI](https://github.com/Kronuz/static-string/actions/workflows/ci.yml/badge.svg)](https://github.com/Kronuz/static-string/actions/workflows/ci.yml)
[![License: BSL-1.0](https://img.shields.io/badge/license-BSL--1.0-blue.svg)](LICENSE)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Header-only](https://img.shields.io/badge/header--only-yes-green.svg)

Compile-time strings for C++20: concatenate literals, characters, and integers into a
constant the compiler builds. Each piece is a `constexpr` value whose length is part of
its type, and `+` folds the pieces into one owning array, which you then use as a
`const char*`, `std::string_view`, or `std::string`. Use it for fixed strings you
want to pay nothing for at runtime: ANSI color codes, table headers, labels, hash seeds.

## ✨ Highlights

- **No runtime cost.** Concatenation, integer rendering, and comparisons run in
  `constexpr`; the result is a constant with its length in its type.
- **No heap.** A literal is referenced in place; a concatenation lives in a fixed
  array.
- **Drops into normal code.** Converts implicitly to `const char*` and
  `std::string_view`, explicitly to `std::string`, and formats with `std::format`
  when `<format>` is available.
- **Integers too.** `to_string<-42>()` is `"-42"`, built by the compiler.
- **One header, no dependencies.**

## 🚀 Quick start

```cpp
#include "static_string.hh"

#include <cstdio>
#include <string_view>

namespace ss = static_string;

constexpr auto greeting = ss::string("Hello, ") + ss::string("world") + '!';
static_assert(greeting.size() == 13);

constexpr auto label = ss::string("count=") + ss::to_string<42>();
static_assert(std::string_view(label) == "count=42");

int main() {
	std::puts(greeting);   // prints Hello, world!
}
```

[`examples/demo.cc`](examples/demo.cc) also builds an ANSI color escape and hands the
same bytes to `std::string` and `std::format`.

## 📦 Installation

Requires a C++20 compiler and CMake 3.14 or newer; no other dependencies. CI builds
and tests it with GCC and Clang on Linux (x86_64, arm64), macOS (arm64, x86_64), and
FreeBSD, plus ASan, UBSan, and TSan builds.

```cmake
include(FetchContent)
FetchContent_Declare(static_string
  GIT_REPOSITORY https://github.com/Kronuz/static-string.git
  GIT_TAG        e0a3d6e7fd3e9101bafab474aba458f3cdd37850)  # or main, which tracks the tip
FetchContent_MakeAvailable(static_string)

target_link_libraries(my_app PRIVATE static_string::static_string)
```

Then `#include "static_string.hh"`. Without CMake, copy `static_string.hh` onto your
include path.

## 🎯 When to use it

Strings whose pieces are all known at compile time: escape sequences, labels, keys
and prefixes built from literals and integer constants.

Not for:

- **Runtime data.** The length is part of the type, so `static_string<13, …>` and
  `static_string<14, …>` are different types. Use `std::string`.
- **Very long concatenation chains.** Each step is a template instantiation, which
  costs compile time.

## 📚 Documentation

- [The Bag of Holding](https://kronuz.github.io/blog/bag-of-holding/):
  the Familiars post that covers `static-string` among the small helpers.
- [docs/API.md](docs/API.md): the two flavors, the factories, the operators, and the
  caveats.
- [ARCHITECTURE.md](ARCHITECTURE.md): how concatenation and integer rendering unfold
  at compile time.
- [CHANGELOG.md](CHANGELOG.md): notable changes.
- [AGENTS.md](AGENTS.md): repository map, conventions, and traps.
- [Catalog entry](https://github.com/Kronuz/cpp-libraries/blob/main/AGENTS.md#static-string).

## 🛠️ Building and testing

```sh
cmake -B build
cmake --build build
ctest --test-dir build
./build/static_string_demo
```

The tests use `assert`, so build without `NDEBUG`.

## 🤝 Contributing

Issues and pull requests are welcome. Read [AGENTS.md](AGENTS.md) first, and the
[catalog's AGENTS.md](https://github.com/Kronuz/cpp-libraries/blob/main/AGENTS.md) for
the family's rules. CI must pass on every platform.

## 📄 License

Boost Software License 1.0; see [LICENSE](LICENSE). The core is Andrzej Krzemienski's
2017 constexpr string, which [entered Xapiand](https://github.com/Kronuz/Xapiand/commit/085939b9fa7581256203bcf162ab27194cc66c31)
with his notice and the full Boost license; Dubalu LLC and Germán Méndez Bravo hold
the copyright on the extensions.
