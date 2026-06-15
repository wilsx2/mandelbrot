#pragma once

#include <wacfrac/types.hpp>
#include <concepts>
#include <mdspan>
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

template <Complex T>
auto square_magnitude(const T& a) {
    return a.real()*a.real() + a.imag()*a.imag();
}

template <typename T>
concept Orbit = requires(T a) {
    square_magnitude(a.z);
    a.iterate();
};

template <Complex T>
struct orbit {
    T z;
    T c;
    orbit(T c): z({0.0, 0.0}), c(c) {}
    void iterate() {
        z = z*z + c;
    }
};

// https://fractalforums.org/index.php?topic=4360.0
struct perturbed_orbit {
    std::complex<double> z;
    std::complex<double> dz;
    std::complex<double> dc;
    const std::vector<std::complex<double>>& ref;
    std::size_t ref_n;

    perturbed_orbit(const std::vector<std::complex<double>>& reference, std::complex<double> dc)
        : z(reference[0])
        , dz({0.0, 0.0})
        , dc(dc)
        , ref(reference)
        , ref_n(0uz)
    {}
    perturbed_orbit(const std::vector<std::complex<double>>& reference, std::complex<double> dz, std::complex<double> dc)
        : z(reference[0] + dz)
        , dz(dz)
        , dc(dc)
        , ref(reference)
        , ref_n(0uz)
    {}
    void iterate() {
        if (ref_n >= ref.size())
            return;

        // Iterate dz
        dz = 2.0 * dz * ref[ref_n] + dz * dz + dc;
        ref_n++;

        // Calculate z
        z = ref[ref_n] + dz;

        // Rebase
        if (square_magnitude(z) < square_magnitude(dz) || ref_n >= ref.size()) {
            dz = z;
            ref_n = 0;
        }
    }
};

template<Orbit T>
auto escape_time(T orb, unsigned int max_n, unsigned int start_n = 0uz) -> unsigned int {
    auto n {start_n};
    while (square_magnitude(orb.z) < 2*2 && n < max_n) {
        orb.iterate();
        n++;
    }
    return n;
}

auto compute_reference(multi_complex c, unsigned int max_iterations) -> std::vector<std::complex<double>>;

// https://www.mrob.com/pub/muency/newtonraphsonmethod.html
auto find_nucleus(std::size_t period, multi_complex c, std::size_t max_iterations) -> multi_complex;
    
}   // namespace wacfrac
