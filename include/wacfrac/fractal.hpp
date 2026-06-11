#pragma once

#include <wacfrac/types.hpp>
#include <concepts>
#include <cmath>

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
auto magnitude(T a) {
    return std::sqrt(square_magnitude(a));
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

// https://fractalforums.org/index.php?topic=4360.0
template<Complex T>
auto escape_time_perturbed(T dc, const std::vector<T>& reference, unsigned int max_iterations) -> unsigned int {
    T dz {0.0, 0.0};
    auto ref_n {0u};
    const auto max_ref = static_cast<unsigned int>(reference.size() - 1);

    for (auto dn {0u}; dn < max_iterations; ++dn) {
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
    }

    return max_iterations;
}

// https://mathr.co.uk/web/m-exterior-distance.html
template<Complex T>
auto distance_estimation(T c, unsigned int max_iterations) -> float {
    T z  {0};
    T dz {0};

    for (auto n {0u}; n < max_iterations; ++n) { //TODO: parameterize escape radius; ought be large
        if (square_magnitude(z) < 2*2)
            return 2.0 * z * std::log10(magnitude(z)) / dz;
        dz = 2.0 * z * dz + 1.0;
        z = z*z + c;
    }
    return 0.f;
}

template<Complex T, Complex U>
auto calculate_orbit(U c, unsigned int max_iterations) -> std::vector<T> {
    U z {0.0, 0.0};
    std::vector<T> orbit { static_cast<T>(z) };

    for (auto n {0u}; n < max_iterations && square_magnitude(z) < 2*2; ++n) {
        z = z*z + c;
        orbit.emplace_back(static_cast<T>(z));
        n++;
    }

    return orbit;
}

}   // namespace wacfrac
