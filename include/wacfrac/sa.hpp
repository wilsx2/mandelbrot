#pragma once

#include <wacfrac/orbit.hpp>
#include <vector>
#include <cstddef>

namespace wacfrac {

template <Complex T = std::complex<long double>>
class series_approximator {
    public:
    series_approximator(const std::vector<T>& reference_orbit, std::size_t coefficient_count);
    void resize(std::size_t coefficient_count);
    void compute_coeffs(std::size_t n);
    void compute_coeffs_while_valid(const std::vector<T>& deltas, double threshold = 1e-6);
    auto approximate_delta_n(T delta_0) const -> T;
    auto is_valid(T delta_0, double threshold = 1e-6) const -> bool;
    auto n() const -> std::size_t;

    private:
    void compute_next_coeffs();

    const std::vector<T>& _reference_orbit;
    std::vector<T> _curr_coeffs;
    std::vector<T> _next_coeffs;
    std::size_t _n;
};

}; // namespace wacfrac
