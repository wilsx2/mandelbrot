#pragma once

#include <wacfrac/types.hpp>
#include <concepts>
#include <utility>
#include <functional>
#include <complex>
#include <vector>
#include <type_traits>

namespace wacfrac
{

template <typename T>
concept Complex = requires(T a, T b) {
    { a.real() };
    { a.imag() };
    T{0.0, 0.0};
    { a + b } -> std::convertible_to<T>;
    { a - b } -> std::convertible_to<T>;
    { a * b } -> std::convertible_to<T>;
    { a / b } -> std::convertible_to<T>;
    { -a } -> std::convertible_to<T>;
    { a += b } -> std::same_as<T&>;
    { a -= b } -> std::same_as<T&>;
    { a *= b } -> std::same_as<T&>;
    { a /= b } -> std::same_as<T&>;
};

template <Complex T>
auto square_magnitude(const T& a) {
    return a.real()*a.real() + a.imag()*a.imag();
}

template <Complex T>
auto magnitude(const T& a) {
    return std::sqrt(square_magnitude(a));
}

template <Complex T>
auto escaped(const T& a) {
    constexpr double escape_radius {8.0};
    return square_magnitude(a) > escape_radius*escape_radius;
}

template <Complex T, std::invocable<T> F>
auto escape_generic(T z0, std::size_t n, std::size_t max_n, F&& next_z) -> std::pair<T, std::size_t> {
    auto z {z0};
    while (n < max_n && !escaped(z)) {
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
auto escape(T c, std::size_t max_n) -> std::pair<T, std::size_t> {
    return escape_generic(T{0.0,0.0}, 0, max_n, std::bind_back(compute_next_z<T>, c));
}

template <Complex T = std::complex<long double>>
auto compute_next_perturbation(const std::vector<T>& ref, std::size_t ref_n, T dc, T dz) -> std::tuple<std::size_t, T, T>;

template <Complex T = std::complex<long double>>
auto rebase_reference(const std::vector<T>& ref, std::size_t ref_n, T dz) -> std::tuple<std::size_t, T, T>;

template <Complex T = std::complex<long double>>
auto escape_perturbed(const std::vector<T>& ref, T dc, std::size_t max_n, T dz = {0.0, 0.0}, std::size_t n = 0) -> std::pair<T, std::size_t>;

template <Complex T = std::complex<long double>>
auto compute_reference(multi_complex c, std::size_t max_n, bool do_escape = true) -> std::vector<T>;

}   // namespace wacfrac
