#pragma once

#include <wacfrac/types.hpp>
#include <concepts>
#include <utility>
#include <functional>

namespace wacfrac
{

template <typename T>
concept Complex = requires(T a, T b) {
    { static_cast<double>(a.real()) } -> std::same_as<double>;
    { static_cast<double>(a.imag()) } -> std::same_as<double>;
    { a + b } -> std::convertible_to<T>;
    { a * b } -> std::convertible_to<T>;
    T{0.0, 0.0};
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

template <Complex T>
auto compute_next_z(const T& z, const T& c) -> T {
    return z*z + c;
}

template <Complex T, std::invocable<T> F>
auto escape_generic(T z0, unsigned int n, unsigned int max_n, F&& next_z) -> std::pair<std::complex<double>, unsigned int> {
    auto z {z0};
    while (n < max_n && !escaped(z)) {
        z = next_z(z);
        ++n;
    }
    return {static_cast<std::complex<double>>(z), n};
}

template<Complex T>
auto escape(T c, unsigned int max_n) -> std::pair<std::complex<double>, unsigned int> {
    return escape_generic(T{0.0,0.0}, 0, max_n, std::bind_back(compute_next_z<T>, c));
}
auto escape_perturbed(const std::vector<std::complex<double>>& ref, std::complex<double> dc, unsigned int max_n, std::complex<double> dz = {0.0, 0.0}, unsigned int n = 0) -> std::pair<std::complex<double>, unsigned int>;
auto compute_reference(multi_complex c, unsigned int max_n) -> std::vector<std::complex<double>>;

auto find_nucleus(std::size_t period, multi_complex c, std::size_t max_iterations) -> multi_complex;

}   // namespace wacfrac
