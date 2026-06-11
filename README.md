# enumstr

Header-only, non-intrusive compile-time enum ⇄ string for C++20. No macros, no
code generation, no dependencies — just include the header. Works on GCC,
Clang, and MSVC by parsing the compiler's function-signature macro.

## Usage

Drop [`include/enumstr.hpp`](include/enumstr.hpp) on your include path and go:

```cpp
#include "enumstr.hpp"

enum class Color { Red, Green, Blue };

enumstr::to_string(Color::Green);          // -> "Green"
enumstr::to_string(static_cast<Color>(9)); // -> "<unknown>"

enumstr::from_string<Color>("Blue");       // -> std::optional{Color::Blue}
enumstr::from_string<Color>("Magenta");    // -> std::nullopt
```

Everything is `constexpr`, so it works at compile time too:

```cpp
static_assert(enumstr::to_string(Color::Red) == "Red");
```

### Values outside [0, 64)

By default only values in `[0, 64)` are scanned. For negative or large
enumerators, specialize `enum_range`:

```cpp
enum class Signal { Lo = -2, Mid = 0, Hi = 7 };

template <>
struct enumstr::enum_range<Signal> {
    static constexpr int min = -4;
    static constexpr int max = 8;
};

enumstr::to_string(Signal::Lo);            // -> "Lo"
```

## Build & test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

Requires a C++20 compiler. CI exercises GCC 12/13/14 and Clang 16/17/18 on
Ubuntu, plus MSVC on Windows.
