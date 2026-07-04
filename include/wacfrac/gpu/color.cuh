#pragma once

#include "wacfrac/color.hpp"
#include <cuda/std/span>
#include <cuda/std/complex>

namespace wacfrac {

namespace gpu {

__device__
auto colorize_discrete(cuda::std::span<const Pixel> palette,
                       std::size_t max_n,
                       cuda::std::span<const std::size_t> n)
                       -> Pixel;

} // namespace gpu

} // namespace gpu
