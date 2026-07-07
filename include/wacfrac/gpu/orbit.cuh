#pragma once

#include "wacfrac/complex_concept.hpp"
#include <cuda/std/span>
#include <cuda/std/complex>
#include <cstddef>
#include <cstdio>

namespace wacfrac {

namespace gpu {

template <Complex T>
__inline__ __device__
auto escaped(T z, float escape_radius) -> bool {
    return norm(z) > escape_radius;
}

template <Complex T>
__inline__ __device__
auto escape(T c, std::size_t max_n, float escape_radius) -> cuda::std::pair<cuda::std::complex<float>, std::size_t> {
    T z {0.0};
    std::size_t n {0};
    while (n < max_n && !escaped(z, escape_radius)) {
        z = z*z + c;
        ++n;
    }
    return cuda::std::make_pair(static_cast<cuda::std::complex<float>>(z), n);
}

} // namespace gpu

} // namespace gpu
