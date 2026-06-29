// A runnable tour of static-string.
//
// Build (when this repo is the top-level project):
//   cmake -B build && cmake --build build && ./build/static_string_demo
//
// The one idea worth taking away: a static_string is built ENTIRELY by the
// compiler. A concatenation of literals, chars, and integers is folded into a
// single constant whose length lives in its type, with nothing left to do at
// runtime. This demo shows that construction, proves it ran at compile time with
// a static_assert, renders integers into the string with to_string<N>(), builds
// an ANSI escape the way term-color does, and then crosses into runtime to hand
// the same bytes to string_view / std::string / std::format.
#include <cstdio>
#include <format>
#include <string>
#include <string_view>

#include "static_string.hh"

using namespace static_string;

static void rule(const char* title) {
	std::printf("\n\033[1m── %s ──\033[0m\n", title);
}

int main() {
	std::puts("static-string demo");

	// --- 1. concatenation, folded by the compiler ----------------------------
	rule("string(...) + ... is one compile-time constant");
	// hello + " " + world is not a runtime concatenation: the compiler unpacks
	// both operands' bytes into one char_array, and its length (11) is part of
	// its type. Nothing here runs at runtime.
	constexpr auto greeting = string("hello") + " " + string("world");
	std::printf("  greeting  : \"%s\"  (%zu chars, length fixed in the type)\n",
	            greeting.c_str(), greeting.size());

	// Mixing a literal, a char, and a static_string in one expression also folds.
	constexpr auto path = string("usr") + '/' + string("bin");
	std::printf("  path      : \"%s\"  (literal + char + static_string)\n", path.c_str());

	// --- 2. proof it really happened at compile time -------------------------
	rule("static_assert: the value exists before main() runs");
	// If any of this were a runtime build, these would not compile. They are
	// compile-time booleans the compiler can fold, not assertions checked later.
	static_assert(greeting.size() == 11, "hello world is 11 chars");
	static_assert(greeting == string("hello world"), "concatenation matches the literal");
	static_assert(string("abc") < string("abd"), "ordering is lexicographic and constexpr");
	std::puts("  all three static_asserts held -> equality and ordering ran in the compiler");

	// --- 3. integers rendered into the string --------------------------------
	rule("to_string<N>() explodes an integer into characters, at compile time");
	// to_string<N>() recursively peels digits off N into a char pack, then packs
	// them into a char_array. The sign is handled, so negatives render cleanly.
	constexpr auto answer = to_string<42>();
	constexpr auto big    = to_string<1234>();
	constexpr auto neg    = to_string<-5>();
	std::printf("  to_string<42>()   -> \"%s\"\n", answer.c_str());
	std::printf("  to_string<1234>() -> \"%s\"\n", big.c_str());
	std::printf("  to_string<-5>()   -> \"%s\"  (sign handled, no INT_MIN overflow)\n", neg.c_str());
	static_assert(to_string<42>() == string("42"), "to_string<42> == \"42\"");

	// --- 4. building an escape / format string at compile time ---------------
	rule("compose a real escape sequence the way term-color does");
	// This is the static-string idiom term-color is built on: stitch an ESC, a
	// '[', some SGR parameters rendered with to_string<N>(), and an 'm' into one
	// constant. The whole sequence is a literal embedded in the binary.
	constexpr auto ESC = string("\033[");
	constexpr auto red_fg =
		ESC + to_string<38>() + ';' + to_string<2>() + ';' +
		to_string<255>() + ';' + to_string<0>() + ';' + to_string<0>() + 'm';
	std::fputs("  red_fg bytes : ", stdout);
	for (char c : std::string_view(red_fg)) {
		if (c == '\033') std::fputs("\\e", stdout);
		else std::putc(c, stdout);
	}
	std::printf("\n  rendered     : %sred%s  (the sequence above, applied)\n",
	            red_fg.c_str(), "\033[0m");

	// --- 5. crossing into runtime --------------------------------------------
	rule("the same constant converts to runtime string types");
	// A static_string converts implicitly to const char* / string_view and
	// explicitly to std::string, and the bundled std::formatter forwards to the
	// string_view formatter, so it drops straight into std::format.
	std::string_view sv = greeting;            // implicit
	std::string      s(greeting);              // explicit
	std::string      f = std::format("[{}]", greeting);  // via the bundled formatter
	std::printf("  string_view : %.*s  (no copy)\n", (int)sv.size(), sv.data());
	std::printf("  std::string : %s  (owns a copy)\n", s.c_str());
	std::printf("  std::format : %s  (formatter forwards to string_view)\n", f.c_str());

	std::puts("\ndone.");
	return 0;
}
