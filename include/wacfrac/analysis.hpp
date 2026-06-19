#pragma once

#include "wacfrac/types.hpp"
#include <cstddef>
#include <vector>

namespace wacfrac {

auto find_nucleus(multi_complex c, std::size_t period, std::size_t max_iterations) -> multi_complex;

auto find_period_ball(multi_complex c0, multi_float dx, multi_float dy, std::size_t max_iterations, bool do_cont) -> std::vector<std::size_t>;

} // namespace wacfrac
