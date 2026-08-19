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

// Scanning an unscoped enum declared without a fixed underlying type casts
// integers it cannot represent. Two compiler eras, two mechanisms:
//
//   Clang 21+ treats such a cast as a hard error, since the result is not a
//   constant expression. detail::representable detects that by substitution and
//   skips those values, so the cast is never made.
//
//   Clang 12 through 20 treat it as a warning instead. Substitution therefore
//   succeeds, representable is always true, and the casts do happen -- this
//   pragma is what keeps them quiet. The names they produce are cast
//   expressions, which detail::valid() rejects.
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunknown-warning-option"  // Clang < 12
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

/// @brief Implementation detail; not part of the public API.
namespace detail {

/// @brief Extracts the unqualified spelling of a single enum value.
///
/// Relies on the compiler embedding @p V in its function-signature macro:
///   - Clang: `std::string_view enumstr::raw_name() [V = Color::Red]`
///   - GCC:   `constexpr std::string_view enumstr::raw_name() [with auto V = Color::Red]`
///   - MSVC:  `...__cdecl enumstr::raw_name<Color::Red>(void)`
///
/// The embedded value is sliced out and any `Type::` qualifier dropped, yielding
/// e.g. `"Red"`. For an unnamed value the compiler emits a cast such as
/// `"(Color)42"` (GCC/Clang) or `"(enum Color)0x2a"` (MSVC) instead; see detail::valid().
/// @tparam V The enum value, passed as a non-type template parameter.
/// @return The enumerator's identifier, or a cast expression for unnamed values.
template <auto V>
constexpr std::string_view raw_name() {
#if defined(__clang__) || defined(__GNUC__)
    constexpr std::string_view sig = __PRETTY_FUNCTION__;
    constexpr auto begin = sig.find("V = ");
    static_assert(begin != std::string_view::npos,
                  "enumstr: __PRETTY_FUNCTION__ is not in the expected format; "
                  "this compiler version needs a new parse case");
    std::string_view s = sig.substr(begin + 4);
    s = s.substr(0, s.find_first_of(";]"));
#elif defined(_MSC_VER)
    constexpr std::string_view sig = __FUNCSIG__;
    constexpr auto begin = sig.find("raw_name<");
    static_assert(begin != std::string_view::npos,
                  "enumstr: __FUNCSIG__ is not in the expected format; "
                  "this compiler version needs a new parse case");
    std::string_view s = sig.substr(begin + 9);
    s = s.substr(0, s.rfind(">("));
#else
#  error "enumstr: unsupported compiler (need GCC, Clang, or MSVC)"
#endif
    if (auto pos = s.rfind("::"); pos != std::string_view::npos)
        s.remove_prefix(pos + 2);
    return s;
}

/// @brief raw_name<V>() as a constant-initialized variable.
///
/// raw_name() is called from ordinary runtime code in to_string() and
/// from_string(), where nothing forces constant evaluation -- an unoptimized
/// build really does re-parse __PRETTY_FUNCTION__ on every call, once per value
/// in the scan window. Binding the result to a variable template evaluates it at
/// compile time regardless of optimization level.
/// @tparam V The enum value to name.
template <auto V>
inline constexpr std::string_view name_v = raw_name<V>();

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
    constexpr std::string_view n = name_v<V>;
    if (n.empty() || (n.front() >= '0' && n.front() <= '9'))
        return false;
    for (char c : n)
        if (!(c == '_' || (c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
            return false;
    return true;
}

/// @brief Whether `static_cast<E>(N)` is a usable constant expression.
///
/// An unscoped enum declared without a fixed underlying type has a value range
/// only as wide as its enumerators need -- `enum Legacy { LA, LB };` covers
/// `[0, 1]`, not the whole scan window. Casting an integer outside that range
/// yields an unspecified value, so it is not a constant expression and cannot be
/// used as a template argument. Clang 21 and later report that as a hard error.
///
/// Substitution failure is the only portable way to ask, since nothing in
/// <type_traits> exposes an enum's range: std::underlying_type_t is the
/// implementation's storage choice, which is far wider.
/// @tparam E The enumeration type.
/// @tparam N The integer to test.
template <typename E, int N>
concept representable =
    requires { typename std::integral_constant<E, static_cast<E>(N)>::type; };

/// @brief Invokes @p f once per representable integer in `enum_range<E>`'s window.
///
/// Each call passes `std::integral_constant<int, N>` rather than a plain `int`,
/// so @p N stays a constant expression usable as a template argument (e.g.
/// `static_cast<E>(ic.value)` followed by `valid<...>()`) inside @p f.
/// @tparam E The enum whose range bounds the iteration.
/// @tparam F Callable accepting `std::integral_constant<int, N>`.
/// @param f Invoked for every value in `[enum_range<E>::min, enum_range<E>::max)`
///          that E can actually represent; the rest are skipped silently.
template <Enum E, typename F>
constexpr void for_each_value(F f) {
    constexpr int lo = enum_range<E>::min, hi = enum_range<E>::max;
    static_assert(lo < hi, "enumstr: enum_range<E>::max must be greater than ::min");
    static_assert(static_cast<long long>(hi) - lo <= 4096,
                  "enumstr: enum_range<E> window exceeds 4096 values; the scan "
                  "instantiates one template per value, so this would be very "
                  "slow to compile");
    [&]<int... Is>(std::integer_sequence<int, Is...>) {
        ([&] {
            if constexpr (representable<E, lo + Is>)
                f(std::integral_constant<int, lo + Is>{});
        }(), ...);
    }(std::make_integer_sequence<int, hi - lo>{});
}

} // namespace detail

/// @brief Converts an enum value to its enumerator name.
/// @tparam E Enumeration type (deduced from @p value).
/// @param value The value to name.
/// @return The enumerator spelling (e.g. `"Red"`), or `"<unknown>"` if @p value
///         is not a declared enumerator within `enum_range<E>`.
template <Enum E>
constexpr std::string_view to_string(E value) {
    std::string_view out = "<unknown>";
    detail::for_each_value<E>([&](auto ic) {
        constexpr E e = static_cast<E>(ic.value);
        if (detail::valid<e>() && e == value)
            out = detail::name_v<e>;
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
    detail::for_each_value<E>([&](auto ic) {
        constexpr E e = static_cast<E>(ic.value);
        if (detail::valid<e>() && detail::name_v<e> == name)
            out = e;
    });
    return out;
}

} // namespace enumstr

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif
