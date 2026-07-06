#pragma once

#include "wacfrac/resolution.hpp"
#include "wacfrac/color.hpp"
#include "wacfrac/complex_concept.hpp"
#include <cuda/std/span>
#include <cuda/std/complex>

namespace wacfrac {

namespace gpu {

template <Complex T>
__device__
auto sample_c_value(std::size_t idx,
                    Resolution res,
                    T delta,
                    float escape_radius) -> T;

template <Complex T>
__global__
void render(Pixel* pixels,
            std::size_t width,
            std::size_t height,
            T start,
            T delta,
            float escape_radius,
            std::size_t max_iterations,
            cuda::std::span<const Pixel> palette);

} // namespace gpu

} // namespace gpu
