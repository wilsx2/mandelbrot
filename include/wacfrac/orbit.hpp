#pragma once

#include <wacfrac/types.hpp>
#include <concepts>
#include <utility>
#include <functional>
#include <complex>
#include <vector>

namespace wacfrac
{

template <Complex T>
auto square_magnitude(const T& a) {
    return a.real()*a.real() + a.imag()*a.imag();
}

template <Complex T>
auto magnitude(const T& a) {
    using std::sqrt;
    using boost::multiprecision::sqrt;
    return sqrt(square_magnitude(a));
}

template <Complex T>
auto escaped(const T& a, double escape_radius = 2.0) {
    return square_magnitude(a) > escape_radius*escape_radius;
}

template <Complex T, std::invocable<T> F>
auto escape_generic(T z, std::size_t n, std::size_t max_n, F&& next_z, double escape_radius = 2.0) -> std::pair<T, std::size_t> {
    while (n < max_n && !escaped(z, escape_radius)) {
        z = next_z(z);
        ++n;
    }
    return {z, n};
}

template <Complex T>
auto compute_next_z(const T& z, const T& c) -> T {
    return z*z + c;
}

template<Complex T>
auto escape(T c, std::size_t max_n, double escape_radius = 2.0) -> std::pair<T, std::size_t> {
    return escape_generic(T{0.0,0.0}, 0, max_n, std::bind_back(compute_next_z<T>, c), escape_radius);
}

template <Complex T = std::complex<long double>>
auto compute_next_perturbation(const std::vector<T>& ref, std::size_t ref_n, T dc, T dz) -> std::tuple<std::size_t, T, T>;

template <Complex T = std::complex<long double>>
auto rebase_reference(const std::vector<T>& ref, std::size_t ref_n, T dz) -> std::tuple<std::size_t, T, T>;

template <Complex T = std::complex<long double>>
auto escape_perturbed(const std::vector<T>& ref, T dc, std::size_t max_n, double escape_radius = 2.0, T dz = {0.0, 0.0}, std::size_t n = 0) -> std::pair<T, std::size_t>;

template <Complex T = std::complex<long double>>
auto compute_reference(MultiComplex c, std::size_t max_n, double escape_radius = 2.0) -> std::vector<T>;

template <Complex T = std::complex<long double>>
auto compute_reference_mt(MultiComplex c, std::size_t max_n, double escape_radius = 2.0) -> std::vector<T>;

struct ReferenceSet {
    std::vector<std::complex<double>> double_ref;
    std::vector<std::complex<long double>> long_double_ref;
    std::vector<DoubleExpComplex> dexp_ref;
};

auto compute_references_all(MultiComplex c, std::size_t max_n, double escape_radius = 2.0) -> ReferenceSet;

}   // namespace wacfrac
