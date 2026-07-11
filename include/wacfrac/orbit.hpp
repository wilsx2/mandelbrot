#pragma once

#include "wacfrac/complex_adapter.hpp"
#include <wacfrac/types.hpp>
#include <concepts>
#include <utility>
#include <functional>
#include <cstddef>
#include <span>


namespace wacfrac
{

template <ComplexConcept T>
WF_HD
inline void compute_next_orbit(T& z, unsigned& n, const T& c) {
    z = z*z + c;
    ++n;
}

template <ComplexConcept T = DoubleComplex>
WF_HD
inline void compute_next_perturbation(T& z, T& dz, unsigned& n, const T& dc, WF_STD::span<const T> ref, unsigned& ref_n) {
    dz = static_cast<ComplexValueTypeT<T>>(2.0) * dz * ref[ref_n] + dz * dz + dc;
    ++ref_n;
    ++n;
}

template <ComplexConcept T = DoubleComplex>
WF_HD
inline void rebase_perturbation(T& z, T& dz, WF_STD::span<const T> ref, unsigned& ref_n) {
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

template <ComplexConcept T>
WF_HD
inline auto escaped(const T& z, double escape_radius) -> bool {
    return static_cast<double>(norm(z)) > escape_radius*escape_radius;
}

template <ComplexConcept T, std::invocable<T&, unsigned&> F>
WF_HD
auto escape_generic(T z, unsigned max_n, double escape_radius, F&& next_orbit) -> WF_STD::pair<Complex<float>, unsigned> {
    unsigned n {0u};
    while (n < max_n && !escaped(z, escape_radius))
        next_orbit(z, n);
    return WF_STD::make_pair(
        SingleComplex{static_cast<float>(z.real()), static_cast<float>(z.imag())}, n);
}

template <ComplexConcept T>
WF_HD
auto escape(const T& c, unsigned max_n, double escape_radius) -> WF_STD::pair<Complex<float>, unsigned> {
    return escape_generic<T>(0.0, max_n, escape_radius,
        [&c](T& z, unsigned& n){
            compute_next_orbit(z, n, c);
        });
}

template <ComplexConcept T>
WF_HD
auto escape_perturbed(const T& dc, WF_STD::span<const T> ref, unsigned max_n, double escape_radius) -> WF_STD::pair<Complex<float>, unsigned> {
    unsigned ref_n {0u};
    T dz {0.0};
    T z {ref[ref_n] + dz};

    return escape_generic<T>(ref[ref_n] + dz, max_n, escape_radius,
        [&](T& z, unsigned& n){
            compute_next_perturbation(z, dz, n, dc, ref, ref_n);
            rebase_perturbation(z, dz, ref, ref_n);
        });
}

}   // namespace wacfrac
