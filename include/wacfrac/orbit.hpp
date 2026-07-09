#pragma once

#include <wacfrac/types.hpp>
#include <concepts>
#include <utility>
#include <functional>
#include <cstddef>

#if defined(__CUDACC__)
    #include <cuda/std/complex>
    #include <cuda/std/span>
    #define STD cuda::std
#else 
    #include <complex>
    #include <span>
    #define STD std
#endif

namespace wacfrac
{

template <Complex T>
#if defined(__CUDACC__)
__forceinline__ __host__ __device__
#else
inline
#endif 
void compute_next_orbit(T& z, std::size_t& n, const T& c) {
    z = z*z + c;
    ++n;
}

template <Complex T = std::complex<double>>
#if defined(__CUDACC__)
__forceinline__ __host__ __device__
#else
inline
#endif 
void compute_next_perturbation(T& z, T& dz, std::size_t& n, const T& dc, STD::span<const T> ref, std::size_t& ref_n) {
    dz = static_cast<ComplexValueTypeT<T>>(2.0) * dz * ref[ref_n] + dz * dz + dc;
    ++ref_n;
    ++n;
}

template <Complex T = std::complex<double>>
#if defined(__CUDACC__)
__forceinline__ __host__ __device__
#else
inline
#endif 
void rebase_perturbation(T& z, T& dz, STD::span<const T> ref, std::size_t& ref_n) {
    if (ref_n >= ref.size()) {
        dz = z;
        ref_n = 0;
        return;
    }

    z = {ref[ref_n] + dz};

    using std::norm;
    if (norm(z) < norm(dz)) {
        dz = z;
        ref_n = 0;
    }
}

template <Complex T>
#if defined(__CUDACC__)
__forceinline__ __host__ __device__
#else
inline
#endif 
auto escaped(const T& z, double escape_radius) -> bool {
    return norm(z) > escape_radius;
}

template <Complex T, std::invocable<T&, std::size_t&> F>
#if defined(__CUDACC__)
__forceinline__ __host__ __device__
#else
inline
#endif 
auto escape_generic(T z, std::size_t max_n, double escape_radius, F&& next_orbit) -> STD::pair<STD::complex<float>, std::size_t> {
    std::size_t n {0};
    while (n < max_n && !escaped(z, escape_radius))
        next_orbit(z, n);
    return STD::make_pair(static_cast<STD::complex<float>>(z), n);
}

template <Complex T>
#if defined(__CUDACC__)
__forceinline__ __host__ __device__
#else
inline
#endif 
auto escape(const T& c, std::size_t max_n, double escape_radius) -> STD::pair<STD::complex<float>, std::size_t> {
    return escape_generic<T>(0.0, max_n, escape_radius,
        [&c](T& z, std::size_t& n){
            compute_next_orbit(z, n, c);
        });
}

template <Complex T>
#if defined(__CUDACC__)
__forceinline__ __host__ __device__
#else
inline
#endif 
auto escape_perturbed(const T& dc, STD::span<const T> ref, std::size_t max_n, double escape_radius) -> STD::pair<STD::complex<float>, std::size_t> {
    auto ref_n {0uz};
    T dz {0.0};
    T z {ref[ref_n] + dz};

    return escape_generic<T>(ref[ref_n] + dz, max_n, escape_radius,
        [&](T& z, std::size_t& n){
            compute_next_perturbation(z, dz, n, dc, ref, ref_n);
            rebase_perturbation(z, dz, ref, ref_n);
        });
}

}   // namespace wacfrac

#undef STD
