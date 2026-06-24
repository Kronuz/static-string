// Smoke test for the standalone static-string library.
// Build: c++ -std=c++17 -I.. test.cc -o test && ./test
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string_view>
#include "static_string.hh"

namespace ss = static_string;

int main() {
	// Compile-time concatenation of literals + a char.
	constexpr auto greeting = ss::string("Hello, ") + ss::string("world") + '!';
	static_assert(greeting.size() == 13, "size");
	assert(std::strcmp(greeting.c_str(), "Hello, world!") == 0);

	// Integer to string, at compile time.
	constexpr auto n = ss::to_string<2025>();
	assert(std::strcmp(n.c_str(), "2025") == 0);

	// Concatenate with a raw string literal.
	constexpr auto path = ss::string("/usr") + "/local";
	assert(std::string_view(path) == "/usr/local");

	// Comparison is constexpr too.
	static_assert(ss::string("abc") == ss::string("abc"), "eq");
	static_assert(!(ss::string("abc") == ss::string("abd")), "neq");

	std::printf("static-string OK: \"%s\" (size %zu), to_string<2025>=%s\n",
	            greeting.c_str(), greeting.size(), n.c_str());
	return 0;
}
