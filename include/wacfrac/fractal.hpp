#pragma once

#include <concepts>
#include <wacfrac/types.hpp>

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

template<Complex T>
auto escape_time(T c, unsigned int max_iterations) -> unsigned int {
    T z {0.0, 0.0};
    auto n = 0u;

    while (z.real()*z.real() + z.imag()*z.imag() < 4 && n < max_iterations) {
        z = z*z + c;
        ++n;
    }

    return n;
}

}   // namespace wacfrac
