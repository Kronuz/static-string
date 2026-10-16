# static-string API reference

Everything lives in `namespace static_string` in `static_string.hh`.

## The value type

```cpp
template <std::size_t N, typename T>
class static_string;
```

`N` is the string length **without** the null terminator. `T` is a tag that
picks one of two flavors (the unspecialized primary template is a hard error):

- `string_literal_ref<N>` is `static_string<N, literal_ref>`. It is a non-owning wrapper over a string literal
  reference, `const char (&)[N + 1]`. No copy, no
  storage of its own. Produced by `string("literal")`.
- `string_char_array<N>` is `static_string<N, char_array>`. It owns a stack-stored, null-terminated `char[N + 1]`. Every concatenation result is a `char_array`.

A `string_literal_ref<N>` converts to `string_char_array<N>` by copying its
characters into the owning buffer.

## Factories

```cpp
template <std::size_t N_PLUS_1>
constexpr auto string(const char (&s)[N_PLUS_1]);   // -> string_literal_ref<N_PLUS_1 - 1>
constexpr auto string(const char ch);               // -> string_char_array<1>
```

`string(...)` wraps a raw string literal as a non-owning `string_literal_ref`
and deduces the length, dropping the null terminator.
The `char` overload produces a one-element owning string.

```cpp
template <int num>  constexpr auto to_string();        // -> string_char_array<digits>
template <char ch>  constexpr auto char_to_string();   // -> string_char_array<1>
```

`to_string<int>()` renders an integer to its decimal digits at compile time,
with a leading `-` for negatives; `to_string<0>()` is `"0"`.
`char_to_string<char>()` makes a one-element owning string from a char.

## Concatenation operators

```cpp
operator+(static_string, static_string)   // -> string_char_array<NL + NR>
operator+(const char (&)[...], static_string)
operator+(static_string, const char (&)[...])
operator+(char, static_string)
operator+(static_string, char)
```

All five overloads return an owning
`string_char_array` whose size is the sum of the operand sizes, computed at
compile time. Operands may be either flavor, raw literals, or single chars in
any combination.

Two extra `operator+` overloads take a
`static_string` on one side and an arbitrary other type on the other and return
a runtime `std::string`. These are the escape hatch for mixing a compile-time
string with a runtime one; they allocate and are not `constexpr`.

## Accessors and conversions

Both flavors expose the same interface:

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

## Notes & caveats

- The length is baked into the type. `static_string<3, ...>` and
  `static_string<4, ...>` are distinct types, so you generally store results in
  `auto` (ideally `constexpr auto`) rather than naming the type.
- `string("literal")` is a non-owning `string_literal_ref`: it holds a reference
  to the literal's storage and copies nothing. The result of any `+`, by
  contrast, is an owning `string_char_array` with its own stack buffer.
- `to_string<N>()` handles any `int`, negatives and `INT_MIN` included.
- Long concatenation chains grow template instantiations and slow the compiler.
  This is a compile-time cost, not a runtime one.
- The `constexpr_assert` checks (bounds, null termination, size match) fire as
  runtime `assert`s outside constant evaluation and are compiled out under
  `NDEBUG`.
- The optional `std::formatter` (C++20) is pure convenience; the implicit
  `std::string_view` conversion already lets `std::format` print a
  `static_string`.
