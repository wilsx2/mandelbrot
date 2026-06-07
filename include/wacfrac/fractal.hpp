#pragma once

#include <wacfrac/types.hpp>

namespace wacfrac
{

auto escape_time(multi_complex c, unsigned int max_iterations) -> unsigned int;

inline auto escape_time_percent(multi_complex c, unsigned int max_iterations) -> float {
    return escape_time(c, max_iterations) / static_cast<float>(max_iterations);
}

}   // namespace wacfrac
