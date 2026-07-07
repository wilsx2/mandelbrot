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

template <Complex T, std::invocable<T&, std::size_t&> F>
__inline__ __device__
auto escape_generic(std::size_t max_n, double escape_radius, F&& next_orbit) -> cuda::std::pair<cuda::std::complex<float>, std::size_t> {
    T z {0.0};
    std::size_t n {0};
    while (n < max_n && !escaped(z, escape_radius))
        next_orbit(z, n);
    return cuda::std::make_pair(static_cast<cuda::std::complex<float>>(z), n);
}

template <Complex T>
__inline__ __device__
auto escape_direct(T c, std::size_t max_n, float escape_radius) -> cuda::std::pair<cuda::std::complex<float>, std::size_t> {
    return escape_generic<T>(max_n, escape_radius,
        [&c](T& z, std::size_t& n){
            z = z*z + c;
            ++n;
        });
}

/*
template <Complex T>
__inline__ __device__
auto escape_perturbed(T dc, cuda::std::span<const T> reference, std::size_t max_n, float escape_radius) -> cuda::std::pair<cuda::std::complex<float>, std::size_t> {
    std::size_t ref_n {0};
    std::size_t n {0};
    T dz {0.0};
    while (n < max_n && !escaped(z, escape_radius)) {
        z = z*z + c;
        ++n;
    }
    return cuda::std::make_pair(static_cast<cuda::std::complex<float>>(z), n);
}
*/

} // namespace gpu

} // namespace gpu
