# enumstr

Header-only C++20 library that converts enum values to their names and names
back to values at compile time, without changes to the enum. It needs no
macros, no code generation, and no dependencies. It parses the
function-signature macro of GCC, Clang, or MSVC.

## Usage

Add [`include/enumstr.hpp`](include/enumstr.hpp) to your include path:

```cpp
#include "enumstr.hpp"

enum class Color { Red, Green, Blue };

enumstr::to_string(Color::Green);               // -> std::string{"Green"}
enumstr::to_string_view(Color::Green);          // -> std::string_view{"Green"}
enumstr::to_string_view(static_cast<Color>(9)); // -> "<unknown>"

enumstr::from_string<Color>("Blue");       // -> std::optional{Color::Blue}
enumstr::from_string<Color>("Magenta");    // -> std::nullopt
```

`to_string_view` and `from_string` are `constexpr`, so they also work at compile
time:

```cpp
static_assert(enumstr::to_string_view(Color::Red) == "Red");
```

### Values outside [0, 64)

By default, enumstr scans only values in `[0, 64)`. For negative or large
enumerators, specialize `enum_range`:

```cpp
enum class Signal { Lo = -2, Mid = 0, Hi = 7 };

template <>
struct enumstr::enum_range<Signal> {
    static constexpr int min = -4;
    static constexpr int max = 8;
};

enumstr::to_string_view(Signal::Lo);       // -> "Lo"
```

## Performance

The compiler resolves names at compile time. At run time, a call compares its
argument against a constant for each enumerator in the scan window.
`tests/bench.cpp` measures this. It passes every input through a `volatile` so
that the compiler cannot fold a call down to its answer:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && ./build/bench
```

## Limitations

enumstr does not support bitmask enums. `to_string_view` matches a value against
declared enumerators one at a time, so `to_string_view(Flags::A | Flags::B)` is
`"<unknown>"` unless the combination is itself an enumerator.

Unscoped enums declared without a fixed underlying type (`enum Legacy { LA, LB };`)
have a value range only as wide as their enumerators need. enumstr names their
enumerators, but casting a value outside that range to the enum has undefined
behavior, before enumstr sees it. UBSan's `-fsanitize=enum` reports the load
from inside the header. To name values that such an enum does not declare, give
it an explicit underlying type (`enum Legacy : int { LA, LB };`).

Compile cost scales with the width of the scan window, not with the number of
enumerators. Every value in `[min, max)` instantiates a template. If you widen
`enum_range` to cover one distant enumerator, you pay for every value in
between, so move the enumerator closer instead. enumstr rejects windows wider
than 4096 values.

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

enumstr requires a C++20 compiler. CI runs GCC 12, 13, and 14 and Clang 16,
17, 18, and 21 on Ubuntu, AppleClang on macOS, and MSVC on Windows. It also
runs the tests under UBSan and ASan.

## Use as a dependency

With `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(enumstr GIT_REPOSITORY <repo-url> GIT_TAG main)
FetchContent_MakeAvailable(enumstr)
target_link_libraries(your_target PRIVATE enumstr::enumstr)
```

After `cmake --install`, call `find_package(enumstr CONFIG REQUIRED)` and link
`enumstr::enumstr`.
