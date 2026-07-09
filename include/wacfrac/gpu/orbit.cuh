#pragma once

#include "wacfrac/complex_concept.hpp"
#include <cuda/std/span>
#include <cuda/std/complex>
#include <cstddef>
#include <cstdio>

namespace wacfrac {

namespace gpu {

constexpr float ESCAPE_RADIUS = 16.0f;

template <Complex T>
__inline__ __device__
auto escaped(T z) -> bool {
    return norm(z) > ESCAPE_RADIUS;
}

template <Complex T, std::invocable<T&, std::size_t&> F>
__inline__ __device__
auto escape_generic(std::size_t max_n, F&& next_orbit) -> cuda::std::pair<cuda::std::complex<float>, std::size_t> {
    T z {0.0};
    std::size_t n {0};
    while (n < max_n && !escaped(z))
        next_orbit(z, n);
    return cuda::std::make_pair(static_cast<cuda::std::complex<float>>(z), n);
}

template <Complex T>
__inline__ __device__
auto escape_direct(T c, std::size_t max_n) -> cuda::std::pair<cuda::std::complex<float>, std::size_t> {
    return escape_generic<T>(max_n,
        [&c](T& z, std::size_t& n){
            z = z*z + c;
            ++n;
        });
}


template <Complex T>
__inline__ __device__
auto escape_perturbed(T dc, cuda::std::span<const T> reference, std::size_t max_n) -> cuda::std::pair<cuda::std::complex<float>, std::size_t> {
    static const T TWO{2.0, 0.0};
    T dz {0.0};
    std::size_t ref_n {0};
    return escape_generic<T>(max_n,
        [&](T& z, std::size_t& n){
            dz = TWO * dz * reference[ref_n] + dz * dz + dc;
            ++ref_n;
            ++n;

            if (ref_n >= reference.size()) {
                z = dz;
                ref_n = 0;
            } else {
                z = {reference[ref_n] + dz};
                if (norm(z) < norm(dz)) {
                    dz = z;
                    ref_n = 0;
                }
            }
        });
}


} // namespace gpu

} // namespace gpu
