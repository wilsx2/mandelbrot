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
auto square_magnitude(T a) {
    return a.real()*a.real() + a.imag()*a.imag();
}

template<Complex T>
auto escape_time(T c, unsigned int max_iterations) -> unsigned int {
    T z {0.0, 0.0};
    auto n {0u};

    while (n < max_iterations && square_magnitude(z) < 2*2) {
        z = z*z + c;
        n++;
    }

    return n;
}

template<Complex T>
auto escape_time_perturbed(T dc, const std::vector<T>& reference, unsigned int max_iterations) -> unsigned int {
    // Canonical algorithm: https://fractalforums.org/index.php?topic=4360.0
    T dz {0.0, 0.0};
    auto dn {0u};
    auto ref_n {0u};
    const auto max_ref = static_cast<unsigned int>(reference.size() - 1);

    while (dn < max_iterations) {
        dz = 2.0 * dz * reference[ref_n] + dz * dz + dc;
        ref_n++;

        T z = reference[ref_n] + dz;

        // Check for escape
        if (square_magnitude(z) > 2*2) {
            return dn + 1;
        }

        // Rebase
        if (square_magnitude(z) < square_magnitude(dz) || ref_n == max_ref) {
            dz = z;
            ref_n = 0;
        }

        dn++;
    }

    return max_iterations;
}

template<Complex T, Complex U>
auto calculate_orbit(U c, unsigned int max_iterations) -> std::vector<T> {
    U z {0.0, 0.0};
    auto n {0uz};
    std::vector<T> orbit { static_cast<T>(z) };


    while (n < max_iterations && square_magnitude(z) < 2*2) {
        z = z*z + c;
        orbit.emplace_back(static_cast<T>(z));
        n++;
    }


    return orbit;
}

}   // namespace wacfrac
