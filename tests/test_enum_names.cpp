// Tests for enumstr.hpp, with compile-time (static_assert) and runtime checks.
#include "enumstr.hpp"

#include <cassert>
#include <cstdio>
#include <string_view>

using namespace std::string_view_literals;

// --- Test enums -------------------------------------------------------------

enum class Color { Red, Green, Blue };           // contiguous from 0
enum class Sparse { A = 0, B = 5, C = 63 };      // gaps, still inside [0,64)
enum class Plain { X, Y, Z };                    // unscoped-style name, scoped enum

// Enums that are not at global scope: the compiler spells unnamed values as
// "(app::Color)42", so stripping the qualifier must not leave "Color)42".
namespace app { enum class Color { Red, Green, Blue }; }
struct Widget { enum class Mode { On, Off }; };

// Deeper and less common scopes. Only the last "::" matters, so nesting depth
// has no effect, but a class template's argument can itself contain "::".
namespace deep { namespace detail { namespace colors { enum class C { Red, Blue }; } } }
namespace { enum class Anon { Hidden }; }
template <typename T> struct Wrap { enum class E { Inner }; };
struct Outer { struct Inner { enum class Deep { Nested }; }; };

// Identifiers close to the edge of the character test in valid().
enum class Awkward { _leading, x__y, trailing_ };
enum class Unicode { café };

// An unscoped enum with no fixed underlying type has a value range only as
// wide as its enumerators need, which is narrower than the [0,64) scan window.
// The tests do not pass it an undeclared value. Casting one in has undefined
// behavior, so there is no defined result to assert.
enum Legacy { LA, LB };

// Enum with values outside the default [0,64) window, so it needs a custom
// range.
enum class Signal { Lo = -2, Mid = 0, Hi = 7 };

template <>
struct enumstr::enum_range<Signal> {
    static constexpr int min = -4;
    static constexpr int max = 8;
};

// --- Compile-time checks ----------------------------------------------------

static_assert(enumstr::to_string_view(Color::Red) == "Red"sv);
static_assert(enumstr::to_string_view(Color::Green) == "Green"sv);
static_assert(enumstr::to_string_view(Color::Blue) == "Blue"sv);

static_assert(enumstr::to_string_view(Sparse::A) == "A"sv);
static_assert(enumstr::to_string_view(Sparse::B) == "B"sv);
static_assert(enumstr::to_string_view(Sparse::C) == "C"sv);

// A value with no matching enumerator is "<unknown>".
static_assert(enumstr::to_string_view(static_cast<Color>(42)) == "<unknown>"sv);

// from_string finds names and rejects unknown ones.
static_assert(enumstr::from_string<Color>("Green"sv) == Color::Green);
static_assert(enumstr::from_string<Sparse>("C"sv) == Sparse::C);
static_assert(enumstr::from_string<Color>("Nope"sv) == std::nullopt);

// Qualified scopes: names still come back unqualified, unnamed values are unknown.
static_assert(enumstr::to_string_view(app::Color::Green) == "Green"sv);
static_assert(enumstr::to_string_view(static_cast<app::Color>(42)) == "<unknown>"sv);
static_assert(enumstr::to_string_view(Widget::Mode::Off) == "Off"sv);
static_assert(enumstr::to_string_view(static_cast<Widget::Mode>(9)) == "<unknown>"sv);
static_assert(enumstr::from_string<app::Color>("Color)42"sv) == std::nullopt);

// Deep nesting still yields the unqualified name, and still rejects non-names.
static_assert(enumstr::to_string_view(deep::detail::colors::C::Blue) == "Blue"sv);
static_assert(enumstr::to_string_view(static_cast<deep::detail::colors::C>(42)) == "<unknown>"sv);
static_assert(enumstr::to_string_view(Anon::Hidden) == "Hidden"sv);
static_assert(enumstr::to_string_view(static_cast<Anon>(42)) == "<unknown>"sv);
static_assert(enumstr::to_string_view(Wrap<Color>::E::Inner) == "Inner"sv);
static_assert(enumstr::to_string_view(static_cast<Wrap<Color>::E>(42)) == "<unknown>"sv);
static_assert(enumstr::to_string_view(Outer::Inner::Deep::Nested) == "Nested"sv);
static_assert(enumstr::to_string_view(static_cast<Outer::Inner::Deep>(42)) == "<unknown>"sv);
static_assert(enumstr::from_string<deep::detail::colors::C>("Blue"sv) == deep::detail::colors::C::Blue);

static_assert(enumstr::to_string_view(Awkward::_leading) == "_leading"sv);
static_assert(enumstr::to_string_view(Awkward::x__y) == "x__y"sv);
static_assert(enumstr::to_string_view(Awkward::trailing_) == "trailing_"sv);
static_assert(enumstr::to_string_view(Unicode::café) == "café"sv);
static_assert(enumstr::from_string<Unicode>("café"sv) == Unicode::café);

// Unscoped enum: scanning it must compile, and names come back unqualified.
static_assert(enumstr::to_string_view(LA) == "LA"sv);
static_assert(enumstr::from_string<Legacy>("LB"sv) == LB);

// Custom range covering negative enumerators.
static_assert(enumstr::to_string_view(Signal::Lo) == "Lo"sv);
static_assert(enumstr::to_string_view(Signal::Mid) == "Mid"sv);
static_assert(enumstr::to_string_view(Signal::Hi) == "Hi"sv);
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
    // to_string_view over a runtime value.
    for (auto [v, name] : {std::pair{Color::Red, "Red"sv},
                           std::pair{Color::Green, "Green"sv},
                           std::pair{Color::Blue, "Blue"sv}}) {
        CHECK(enumstr::to_string_view(v) == name);
    }

    // Round-trip one enumerator of each enum.
    CHECK(enumstr::from_string<Color>(enumstr::to_string_view(Color::Red)) == Color::Red);
    CHECK(enumstr::from_string<Sparse>(enumstr::to_string_view(Sparse::B)) == Sparse::B);
    CHECK(enumstr::from_string<Plain>(enumstr::to_string_view(Plain::Y)) == Plain::Y);

    // Owning names compose with std::string.
    CHECK("color " + enumstr::to_string(Color::Green) == "color Green");
    CHECK(enumstr::to_string(static_cast<Color>(42)) == "<unknown>");

    // Unknown handling.
    CHECK(enumstr::to_string_view(static_cast<Sparse>(3)) == "<unknown>"sv);
    CHECK(enumstr::from_string<Color>("Magenta"sv) == std::nullopt);

    CHECK(enumstr::from_string<app::Color>(enumstr::to_string_view(app::Color::Blue)) == app::Color::Blue);
    CHECK(enumstr::to_string_view(static_cast<app::Color>(42)) == "<unknown>"sv);

    CHECK(enumstr::to_string_view(LB) == "LB"sv);

    // Empty string never matches.
    CHECK(enumstr::from_string<Color>(""sv) == std::nullopt);

    if (failures == 0) {
        std::printf("All runtime checks passed.\n");
        return 0;
    }
    std::printf("%d runtime check(s) failed.\n", failures);
    return 1;
}
