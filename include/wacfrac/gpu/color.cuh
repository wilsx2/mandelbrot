#pragma once

#include <cstddef>
#include "wacfrac/color.hpp"
#include <cuda/std/span>
#include <cuda/std/complex>

namespace wacfrac::gpu {

__inline__ __device__
auto colorize(cuda::std::complex<float> z,
                         std::size_t n,
                         std::size_t max_n,
                         cuda::std::span<const Pixel> palette) -> Pixel {
    if (n == max_n)
        return palette.back();
    auto cont_n {n - log(log(abs(z))) / log(2.0)};
    auto n1     {static_cast<std::size_t>(std::floor(cont_n))};
    auto n2     {static_cast<std::size_t>(std::ceil(cont_n))};
    auto color1 {palette.at(n1 % palette.size())};
    auto color2 {palette.at(n2 % palette.size())};
    auto progress {std::fmod(cont_n, 1.0)};
    return {
        static_cast<uint8_t>(color2.r * progress + color1.r * (1.0 - progress)),
        static_cast<uint8_t>(color2.g * progress + color1.g * (1.0 - progress)),
        static_cast<uint8_t>(color2.b * progress + color1.b * (1.0 - progress))
    };
}

} // namespace wacfrac::gpu
