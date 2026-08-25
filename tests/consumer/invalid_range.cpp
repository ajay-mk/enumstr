#include <enumstr.hpp>

enum class Bad { Value };

template <>
struct enumstr::enum_range<Bad> {
    static constexpr int min = 2;
    static constexpr int max = 1;
};

static_assert(enumstr::to_string(Bad::Value) == "Value");
