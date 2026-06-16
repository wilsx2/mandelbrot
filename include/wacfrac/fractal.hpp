#pragma once

#include <wacfrac/types.hpp>
#include <concepts>
#include <utility>

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
    return std::sqrt(a.real()*a.real() + a.imag()*a.imag());
}

template <typename T>
concept Orbit = requires(T a) {
    square_magnitude(a.z);
    a.iterate();
    { a.n } -> std::convertible_to<std::size_t>;
};

template <Complex T>
struct orbit {
    T z;
    T c;
    std::size_t n;
    orbit(T c): z({0.0, 0.0}), c(c), n(0) {}
    void iterate() {
        z = z*z + c;
        ++n;
    }
};

// https://fractalforums.org/index.php?topic=4360.0
struct perturbed_orbit {
    std::complex<double> z;
    std::complex<double> dz;
    std::complex<double> dc;
    std::size_t n;
    const std::vector<std::complex<double>>& ref;
    std::size_t ref_n;

    perturbed_orbit(const std::vector<std::complex<double>>& reference, std::complex<double> dc)
        : z(reference[0])
        , dz({0.0, 0.0})
        , dc(dc)
        , n(0uz)
        , ref(reference)
        , ref_n(0uz)
    {}
    perturbed_orbit(const std::vector<std::complex<double>>& reference, std::complex<double> dz, std::complex<double> dc, std::size_t n)
        : z(reference[n] + dz)
        , dz(dz)
        , dc(dc)
        , n(n)
        , ref(reference)
        , ref_n(n)
    {}
    void iterate() {
        if (ref_n >= ref.size())
            return;

        // Iterate dz
        dz = 2.0 * dz * ref[ref_n] + dz * dz + dc;
        ref_n++;
        z = ref[ref_n] + dz;

        // Rebase
        if (square_magnitude(z) < square_magnitude(dz) || ref_n >= ref.size()) {
            dz = z;
            ref_n = 0;
        }

        ++n;
    }
};

template<Orbit T>
auto escape(T orb, unsigned int max_n) -> std::pair<std::complex<double>, unsigned int> {
    while (square_magnitude(orb.z) < 2*2 && orb.n < max_n) {
        orb.iterate();
    }
    return std::make_pair(static_cast<std::complex<double>>(orb.z), static_cast<unsigned int>(orb.n));
}

auto compute_reference(multi_complex c, unsigned int max_iterations) -> std::vector<std::complex<double>>;

// https://www.mrob.com/pub/muency/newtonraphsonmethod.html
auto find_nucleus(std::size_t period, multi_complex c, std::size_t max_iterations) -> multi_complex;

}   // namespace wacfrac
