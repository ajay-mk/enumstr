// Tests for enumstr.hpp — mix of compile-time (static_assert) and runtime checks.
#include "enumstr.hpp"

#include <cassert>
#include <cstdio>
#include <string_view>

using namespace std::string_view_literals;

// --- Test enums -------------------------------------------------------------

enum class Color { Red, Green, Blue };           // contiguous from 0
enum class Sparse { A = 0, B = 5, C = 63 };      // gaps, still inside [0,64)
enum class Plain { X, Y, Z };                    // unscoped-style name, scoped enum

// Enum with values outside the default [0,64) window -> needs custom range.
enum class Signal { Lo = -2, Mid = 0, Hi = 7 };

template <>
struct enumstr::enum_range<Signal> {
    static constexpr int min = -4;
    static constexpr int max = 8;
};

// --- Compile-time checks ----------------------------------------------------

static_assert(enumstr::to_string(Color::Red) == "Red"sv);
static_assert(enumstr::to_string(Color::Green) == "Green"sv);
static_assert(enumstr::to_string(Color::Blue) == "Blue"sv);

static_assert(enumstr::to_string(Sparse::A) == "A"sv);
static_assert(enumstr::to_string(Sparse::B) == "B"sv);
static_assert(enumstr::to_string(Sparse::C) == "C"sv);

// Value with no matching enumerator -> "<unknown>".
static_assert(enumstr::to_string(static_cast<Color>(42)) == "<unknown>"sv);

// Round-trip: from_string(to_string(v)) == v.
static_assert(enumstr::from_string<Color>("Green"sv) == Color::Green);
static_assert(enumstr::from_string<Sparse>("C"sv) == Sparse::C);
static_assert(enumstr::from_string<Color>("Nope"sv) == std::nullopt);

// Custom range covering negative enumerators.
static_assert(enumstr::to_string(Signal::Lo) == "Lo"sv);
static_assert(enumstr::to_string(Signal::Mid) == "Mid"sv);
static_assert(enumstr::to_string(Signal::Hi) == "Hi"sv);
static_assert(enumstr::from_string<Signal>("Lo"sv) == Signal::Lo);

// --- Runtime checks ---------------------------------------------------------

static int failures = 0;

#define CHECK(expr)                                                            \
    do {                                                                       \
        if (!(expr)) {                                                         \
            std::printf("FAIL: %s  (line %d)\n", #expr, __LINE__);             \
            ++failures;                                                        \
        }                                                                      \
    } while (0)

int main() {
    // to_string over a runtime value.
    for (auto [v, name] : {std::pair{Color::Red, "Red"sv},
                           std::pair{Color::Green, "Green"sv},
                           std::pair{Color::Blue, "Blue"sv}}) {
        CHECK(enumstr::to_string(v) == name);
    }

    // Round-trip every enumerator.
    CHECK(enumstr::from_string<Color>(enumstr::to_string(Color::Red)) == Color::Red);
    CHECK(enumstr::from_string<Sparse>(enumstr::to_string(Sparse::B)) == Sparse::B);
    CHECK(enumstr::from_string<Plain>(enumstr::to_string(Plain::Y)) == Plain::Y);

    // Unknown handling.
    CHECK(enumstr::to_string(static_cast<Sparse>(3)) == "<unknown>"sv);
    CHECK(enumstr::from_string<Color>("Magenta"sv) == std::nullopt);

    // Empty string never matches.
    CHECK(enumstr::from_string<Color>(""sv) == std::nullopt);

    if (failures == 0) {
        std::printf("All runtime checks passed.\n");
        return 0;
    }
    std::printf("%d runtime check(s) failed.\n", failures);
    return 1;
}
