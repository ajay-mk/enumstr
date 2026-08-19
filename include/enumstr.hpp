/// @file enumstr.hpp
/// @brief Non-intrusive, dependency-free compile-time enum⇄string conversion.
///
/// Works on GCC, Clang, and MSVC by parsing the compiler's function-signature
/// macro (`__PRETTY_FUNCTION__` / `__FUNCSIG__`). No macros, no code
/// generation, and nothing to add to your enum definitions. Enumerators must
/// lie within enumstr::enum_range (default `[0, 64)`); specialize it otherwise.
#pragma once

#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

// The scan casts every integer in the window to E. An unscoped enum with no
// fixed underlying type has a value range only as wide as its enumerators need,
// so most of those casts are out of range -- unspecified since C++17, but an
// error by default under Clang 16+. The names they produce are cast
// expressions, which valid() rejects anyway.
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wenum-constexpr-conversion"
#endif

namespace enumstr {

/// @brief Constrains the public API to enumeration types.
template <typename E>
concept Enum = std::is_enum_v<E>;

/// @brief Inclusive-min / exclusive-max scan window for an enum's values.
///
/// to_string() and from_string() probe every integer in `[min, max)`. The
/// default covers `[0, 64)`; specialize for enums with negative or large
/// values:
/// @code
/// template <> struct enumstr::enum_range<Signal> {
///     static constexpr int min = -4;
///     static constexpr int max = 8;
/// };
/// @endcode
/// @tparam E The enumeration type the window applies to.
template <typename E>
struct enum_range {
    static constexpr int min = 0;   ///< First value probed (inclusive).
    static constexpr int max = 64;  ///< One past the last value probed (exclusive).
};

/// @brief Extracts the unqualified spelling of a single enum value.
///
/// Relies on the compiler embedding @p V in its function-signature macro:
///   - Clang: `std::string_view enumstr::raw_name() [V = Color::Red]`
///   - GCC:   `constexpr std::string_view enumstr::raw_name() [with auto V = Color::Red]`
///   - MSVC:  `...__cdecl enumstr::raw_name<Color::Red>(void)`
///
/// The embedded value is sliced out and any `Type::` qualifier dropped, yielding
/// e.g. `"Red"`. For an unnamed value the compiler emits a cast such as
/// `"(Color)42"` (GCC/Clang) or `"(enum Color)0x2a"` (MSVC) instead; see valid().
/// @tparam V The enum value, passed as a non-type template parameter.
/// @return The enumerator's identifier, or a cast expression for unnamed values.
template <auto V>
constexpr std::string_view raw_name() {
#if defined(__clang__) || defined(__GNUC__)
    std::string_view s = __PRETTY_FUNCTION__;
    s.remove_prefix(s.find("V = ") + 4);
    s = s.substr(0, s.find_first_of(";]"));
#elif defined(_MSC_VER)
    std::string_view s = __FUNCSIG__;
    s.remove_prefix(s.find("raw_name<") + 9);
    s = s.substr(0, s.rfind(">("));
#else
#  error "enumstr: unsupported compiler (need GCC, Clang, or MSVC)"
#endif
    if (auto pos = s.rfind("::"); pos != std::string_view::npos)
        s.remove_prefix(pos + 2);
    return s;
}

/// @brief Tests whether @p V corresponds to a declared enumerator.
///
/// A declared enumerator's raw_name() is an identifier; an unnamed value yields
/// a cast expression instead. The whole string must be checked, not just its
/// first character: for an enum outside global scope the cast is spelled
/// `"(app::Color)42"`, and dropping the qualifier leaves `"Color)42"`, which
/// starts with an identifier character but is not one.
/// @tparam V The enum value to test.
/// @return `true` if @p V names a real enumerator, `false` otherwise.
template <auto V>
constexpr bool valid() {
    constexpr std::string_view n = raw_name<V>();
    if (n.empty() || (n.front() >= '0' && n.front() <= '9'))
        return false;
    for (char c : n)
        if (!(c == '_' || (c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
            return false;
    return true;
}

/// @brief Invokes @p f once per integer in `enum_range<E>`'s window.
///
/// Each call passes `std::integral_constant<int, N>` rather than a plain `int`,
/// so @p N stays a constant expression usable as a template argument (e.g.
/// `static_cast<E>(ic.value)` followed by `valid<...>()`) inside @p f.
/// @tparam E The enum whose range bounds the iteration.
/// @tparam F Callable accepting `std::integral_constant<int, N>`.
/// @param f Invoked for every value in `[enum_range<E>::min, enum_range<E>::max)`.
template <Enum E, typename F>
constexpr void for_each_value(F f) {
    constexpr int lo = enum_range<E>::min, hi = enum_range<E>::max;
    [&]<int... Is>(std::integer_sequence<int, Is...>) {
        (f(std::integral_constant<int, lo + Is>{}), ...);
    }(std::make_integer_sequence<int, hi - lo>{});
}

/// @brief Converts an enum value to its enumerator name.
/// @tparam E Enumeration type (deduced from @p value).
/// @param value The value to name.
/// @return The enumerator spelling (e.g. `"Red"`), or `"<unknown>"` if @p value
///         is not a declared enumerator within `enum_range<E>`.
template <Enum E>
constexpr std::string_view to_string(E value) {
    std::string_view out = "<unknown>";
    for_each_value<E>([&](auto ic) {
        constexpr E e = static_cast<E>(ic.value);
        if (valid<e>() && e == value)
            out = raw_name<e>();
    });
    return out;
}

/// @brief Converts an enumerator name back to its enum value.
/// @tparam E Enumeration type; specify explicitly, e.g. `from_string<Color>("Red")`.
/// @param name The enumerator spelling to look up.
/// @return The matching value, or `std::nullopt` if no enumerator in
///         `enum_range<E>` has that name.
template <Enum E>
constexpr std::optional<E> from_string(std::string_view name) {
    std::optional<E> out;
    for_each_value<E>([&](auto ic) {
        constexpr E e = static_cast<E>(ic.value);
        if (valid<e>() && raw_name<e>() == name)
            out = e;
    });
    return out;
}

} // namespace enumstr

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif
