#include <enumstr.hpp>

enum class Wide { Value };

template <>
struct enumstr::enum_range<Wide> {
    static constexpr int min = 0;
    static constexpr int max = 4097;
};

static_assert(enumstr::to_string(Wide::Value) == "Value");
