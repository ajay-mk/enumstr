// Tests for enum_names.hpp — mix of compile-time (static_assert) and runtime checks.
#include "enum_names.hpp"

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
struct refl::enum_range<Signal> {
    static constexpr int min = -4;
    static constexpr int max = 8;
};

// --- Compile-time checks ----------------------------------------------------

static_assert(refl::to_string(Color::Red) == "Red"sv);
static_assert(refl::to_string(Color::Green) == "Green"sv);
static_assert(refl::to_string(Color::Blue) == "Blue"sv);

static_assert(refl::to_string(Sparse::A) == "A"sv);
static_assert(refl::to_string(Sparse::B) == "B"sv);
static_assert(refl::to_string(Sparse::C) == "C"sv);

// Value with no matching enumerator -> "<unknown>".
static_assert(refl::to_string(static_cast<Color>(42)) == "<unknown>"sv);

// Round-trip: from_string(to_string(v)) == v.
static_assert(refl::from_string<Color>("Green"sv) == Color::Green);
static_assert(refl::from_string<Sparse>("C"sv) == Sparse::C);
static_assert(refl::from_string<Color>("Nope"sv) == std::nullopt);

// Custom range covering negative enumerators.
static_assert(refl::to_string(Signal::Lo) == "Lo"sv);
static_assert(refl::to_string(Signal::Mid) == "Mid"sv);
static_assert(refl::to_string(Signal::Hi) == "Hi"sv);
static_assert(refl::from_string<Signal>("Lo"sv) == Signal::Lo);

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
        CHECK(refl::to_string(v) == name);
    }

    // Round-trip every enumerator.
    CHECK(refl::from_string<Color>(refl::to_string(Color::Red)) == Color::Red);
    CHECK(refl::from_string<Sparse>(refl::to_string(Sparse::B)) == Sparse::B);
    CHECK(refl::from_string<Plain>(refl::to_string(Plain::Y)) == Plain::Y);

    // Unknown handling.
    CHECK(refl::to_string(static_cast<Sparse>(3)) == "<unknown>"sv);
    CHECK(refl::from_string<Color>("Magenta"sv) == std::nullopt);

    // Empty string never matches.
    CHECK(refl::from_string<Color>(""sv) == std::nullopt);

    if (failures == 0) {
        std::printf("All runtime checks passed.\n");
        return 0;
    }
    std::printf("%d runtime check(s) failed.\n", failures);
    return 1;
}
