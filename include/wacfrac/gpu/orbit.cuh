#pragma once

#include "wacfrac/types.hpp"
#include <cuda/std/span>
#include <cuda/std/complex>

namespace wacfrac {

namespace gpu {

template <Complex T>
__device__
auto escaped(T z, float escape_radius) -> bool;

template <Complex T>
__device__
auto escape(T c, std::size_t max_n, float escape_radius) -> std::size_t;

} // namespace gpu

} // namespace gpu
