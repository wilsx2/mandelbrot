#pragma once

#include "wacfrac/types.hpp"
#include <cstddef>

namespace wacfrac {

auto find_nucleus(std::size_t period, multi_complex c, std::size_t max_iterations) -> multi_complex;

}
