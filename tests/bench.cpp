// Timing for to_string/from_string. Not a correctness test and not run by
// ctest -- build it and run it by hand when changing how the scan works.
//
// Every input goes through a volatile, so the compiler cannot fold a call down
// to its answer. That matters here: the names themselves are compile-time
// constants, and without the barrier an optimizing build reports times for code
// it never emitted.
#include "enumstr.hpp"

#include <chrono>
#include <cstdio>

enum class Color { Red, Green, Blue };       // hits early in the scan window
enum class Sparse { A = 0, B = 31, C = 63 }; // hits late

constexpr int kIters = 1'000'000;

static volatile int opaque_index = 0;
static const char* volatile opaque_name = "Blue";

template <typename F>
static void time_it(const char* label, F body) {
    const auto start = std::chrono::steady_clock::now();
    std::size_t sink = 0;
    for (int i = 0; i < kIters; ++i)
        sink += body(i);
    const auto elapsed = std::chrono::steady_clock::now() - start;
    std::printf("%-34s %7.2f ns/call%s\n", label,
                std::chrono::duration<double, std::nano>(elapsed).count() / kIters,
                sink ? "" : " (sink empty?)");
}

int main() {
    std::printf("%d iterations each\n\n", kIters);

    time_it("to_string, early hit", [](int i) {
        opaque_index = i % 3;
        return enumstr::to_string(static_cast<Color>(opaque_index)).size();
    });
    time_it("to_string, late hit", [](int i) {
        opaque_index = (i % 2) ? 31 : 63;
        return enumstr::to_string(static_cast<Sparse>(opaque_index)).size();
    });
    time_it("to_string, miss", [](int) {
        opaque_index = 42;
        return enumstr::to_string(static_cast<Color>(opaque_index)).size();
    });
    time_it("from_string, hit", [](int) {
        return enumstr::from_string<Color>(opaque_name).has_value() ? 1u : 0u;
    });
    time_it("from_string, miss", [](int) {
        opaque_name = "Magenta";
        const auto found = enumstr::from_string<Color>(opaque_name).has_value();
        opaque_name = "Blue";
        return found ? 0u : 1u;
    });
}
