#pragma once

#include <cstddef>
#include "wacfrac/color.hpp"
#include <cuda/std/span>
#include <cuda/std/complex>

namespace wacfrac::gpu {

__inline__ __device__
auto colorize_discrete(std::size_t n,
                       std::size_t max_n,
                       cuda::std::span<const Pixel> palette) -> Pixel
{
    if (n == max_n)
        return palette.back();
    return palette.at(n % palette.size());
}

} // namespace wacfrac::gpu
