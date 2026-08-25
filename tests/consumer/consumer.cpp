#include <enumstr.hpp>

#include <cassert>

enum class Color { Red, Green, Blue };

int main() {
    assert(enumstr::to_string(Color::Green) == "Green");
}
