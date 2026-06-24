# Architecture

How `static_string` works internally. The whole library is `static_string.hh`.

## Two flavors behind one template

The value type is one class template specialized two ways:

```cpp
template <std::size_t N, typename T>
class static_string;   // primary template is a hard error
```

The primary template static_asserts on an always-false condition
(`static_string.hh:60`), so an unspecialized `static_string<N, T>` never
compiles. Two tag types select the real implementations:

- `literal_ref` (`static_string.hh:65`) gives `string_literal_ref<N>`, a
  **non-owning** view. Its only member is `const char (&_data)[N + 1]`
  (`static_string.hh:78`), a reference to a string literal's storage. It copies
  nothing and stores nothing of its own. The constructor asserts the referenced
  array is null-terminated at index `N` (`static_string.hh:81`).
- `char_array` (`static_string.hh:69`) gives `string_char_array<N>`, an
  **owning** value. It holds `char _data[N + 1]` (`static_string.hh:155`), a
  stack buffer with room for the null terminator. This is where concatenation
  results live.

In both, `N` is the visible length and the buffer is `N + 1` to hold the
trailing `0`. The accessor and comparison surface is duplicated across the two
specializations so they behave identically (`static_string.hh:84` and `:185`).
A `string_literal_ref<N>` converts to `string_char_array<N>` through a copying
constructor (`static_string.hh:176`) when an owning value is needed.

## Concatenation: sizes added at compile time

The owning `char_array` is built from pieces by a family of private constructors
driven by `std::integer_sequence`. The core two-operand constructor takes a
left `static_string<M, TL>`, a right `static_string<N - M, TR>`, and two index
sequences, then splices the characters into the new buffer with a single
brace-initializer (`static_string.hh:159`):

```cpp
// Il... indexes the left operand, Ir... the right; the trailing 0 terminates.
: _data{l[Il]..., r[Ir]..., 0} { }
```

The public constructor (`static_string.hh:172`) supplies
`std::make_integer_sequence<std::size_t, M>` and
`std::make_integer_sequence<std::size_t, N - M>`, so the split point `M` and the
total `N` are template parameters fixed at compile time. There are no loops and
no runtime copying in `constexpr` context: the result is a pack expansion of
`operator[]` calls into an aggregate initializer.

The `operator+` overloads (`static_string.hh:246`-`269`) are thin wrappers that
pick `NL + NR` (or the equivalent for raw literals and chars) as the result size
and call into that constructor:

```cpp
template <std::size_t NL, typename TL, std::size_t NR, typename TR>
constexpr auto operator+(const static_string<NL, TL>& l, const static_string<NR, TR>& r) {
    return string_char_array<NL + NR>(l, r);   // size known here, at compile time
}
```

Raw string literals are adapted by wrapping them in a `string_literal_ref` of
the right deduced length before forwarding to the same machinery
(`static_string.hh:251`, `:256`), and single chars by wrapping them in a
`string_char_array<1>` (`static_string.hh:261`, `:266`). Because every size is a
compile-time constant, the type of a concatenation chain is fully determined by
the compiler and the buffer is sized exactly.

Two further `operator+` overloads (`static_string.hh:271`, `:279`) fall back to a
runtime `std::string` when one side is some arbitrary type rather than a
`static_string`. These allocate and are deliberately not `constexpr`; they exist
so a compile-time string can be glued to a runtime one without ceremony.

## Integers to strings: the explode template

`to_string<int>()` turns an integer literal into its decimal characters at
compile time using a recursive template, `explode<num, N, a...>`
(`static_string.hh:124`):

```cpp
template <int num, std::size_t N = 0, char... a>
struct explode : explode<num / 10, N + 1, ('0' + num % 10), a...> { };
```

Each instantiation peels the least significant digit (`num % 10`), prepends it
to the accumulating char pack `a...`, and recurses on `num / 10` with the
length counter `N` bumped by one. Prepending rather than appending is what puts
the digits back in the right order, since recursion processes them
least-significant first. The recursion bottoms out at `num == 0`
(`static_string.hh:127`), which materializes the collected pack into a
`char value[N + 1]{a..., 0}` static array. A dedicated `explode<0, 0>`
specialization (`static_string.hh:132`) handles the input `0`, emitting `"0"`
rather than the empty string the general base case would give.

`to_string<int>()` then constructs a `string_char_array<sizeof(value) - 1>` from
that static array (`static_string.hh:137`). `char_to_string<char>()` is the
trivial case, a one-element owning string (`static_string.hh:144`).

## constexpr_assert

`constexpr_assert` (`static_string.hh:53`) is the assertion used in constructors
and `operator[]`. Inside constant evaluation a failed check makes the expression
non-constant and so fails the compile; outside it (or in a runtime path) it
expands to a normal `assert`. Under `NDEBUG` it expands to `void(0)` and
vanishes. It is written as an immediately-invoked lambda so it can sit inside a
comma expression in a `constexpr` constructor's member initializer, for example
the null-termination check (`static_string.hh:81`) and the bounds check
(`static_string.hh:89`).

## Complexity

Everything that matters is compile-time. Concatenation, integer rendering, and
comparison all run in `constexpr` context with no heap allocation; the owning
buffer is a stack `char[N + 1]` sized exactly to the result. At runtime a
finished value is just that fixed array, and the conversions to `const char*`,
`std::string_view`, and `std::string` are the obvious O(N) reads or copies
(`static_string.hh:111`, `:213`). The only build-time cost is template
instantiation, which scales with the length and depth of the concatenation
chains.

## Design decisions

The split into a non-owning `literal_ref` and an owning `char_array` keeps
literals zero-copy while still giving concatenation a real buffer to build into.
Carrying the length in the type (`N`) is what makes the result usable in a
`constexpr` and lets `==` short-circuit on length, but it also means the type
changes with every size. Building the buffer with `integer_sequence` pack
expansion instead of a loop keeps the constructors valid in C++17 `constexpr`
without relying on loop support that the earlier design predates. The optional
`std::formatter` (C++20) is gated on `__cpp_lib_format` + `__has_include(<format>)`
so the header has no hard dependency.

## Limitations

- Fixed sizes are baked into the type, so values are kept in `auto` and the type
  is impractical to name by hand.
- Large or deeply nested concatenations grow template instantiations and slow
  the compiler.
- `to_string` is non-negative only; there is no sign handling.
- The `+` overloads require the operand types to line up (a `static_string`, a
  raw literal, or a `char`); arbitrary types route to the runtime
  `std::string` overloads instead of composing at compile time.
