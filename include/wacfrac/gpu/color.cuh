#pragma once

#include "wacfrac/color.hpp"
#include <cuda/std/span>
#include <cuda/std/complex>

namespace wacfrac::gpu {

__inline__ __device__
auto colorize_discrete(cuda::std::span<const Pixel> palette,
                       std::size_t max_n,
                       std::size_t n) -> Pixel
{
    if (n == max_n)
        return palette.back();
    return palette.at(n % palette.size());
}

} // namespace wacfrac::gpu
