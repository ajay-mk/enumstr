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

## Performance

Names are resolved at compile time; a call is a scan over the window comparing
against constants. `tests/bench.cpp` measures it, with every input passed
through a `volatile` so the compiler cannot fold a call down to its answer:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && ./build/bench
```

Run the benchmark on the target compiler and hardware when performance matters.

## Limitations

Bitmask enums are not supported. `to_string` matches a value against declared
enumerators one at a time, so `to_string(Flags::A | Flags::B)` is `"<unknown>"`
unless the combination is itself an enumerator.

Unscoped enums declared without a fixed underlying type (`enum Legacy { LA, LB };`)
have a value range only as wide as their enumerators need. Naming their
enumerators works, but handing `to_string` a value outside that range is
undefined behavior before it ever reaches enumstr, and UBSan's `-fsanitize=enum`
reports the load from inside the header. Give such an enum an explicit
underlying type (`enum Legacy : int { LA, LB };`) if you need to name values it
never declared.

Cost scales with the width of the scan window, not with the number of
enumerators. Every value in `[min, max)` instantiates a template. Widening
`enum_range` to cover one distant enumerator makes you pay for every value in
between, so prefer moving the enumerator to keeping the window wide. Windows
over 4096 values are rejected.

## Build & test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

Requires a C++20 compiler. CI exercises GCC 12/13/14 and Clang 16/17/18 on
Ubuntu, plus MSVC on Windows.

## Use as a dependency

With `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(enumstr GIT_REPOSITORY <repo-url> GIT_TAG main)
FetchContent_MakeAvailable(enumstr)
target_link_libraries(your_target PRIVATE enumstr::enumstr)
```

Or after `cmake --install`, via `find_package(enumstr CONFIG REQUIRED)` and link
`enumstr::enumstr`.
