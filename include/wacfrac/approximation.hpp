#pragma once

#include <vector>
#include <complex>
#include <functional>
#include <cstddef>

namespace wacfrac {

class series_approximator {
    public:
    series_approximator(const std::vector<std::complex<double>>& reference_orbit, std::size_t coefficient_count);
    void resize(std::size_t coefficient_count);
    void compute_coeffs(std::size_t n);
    void compute_coeffs_while_valid(const std::vector<std::complex<double>>& deltas, double threshold = 1e-6);
    auto approximate_delta_n(std::complex<double> delta_0) const -> std::complex<double>;
    auto is_valid(std::complex<double> delta_0, double threshold = 1e-6) const -> bool;
    auto n() const -> std::size_t;

    private:
    void compute_next_coeffs();

    const std::vector<std::complex<double>>& _reference_orbit;
    std::vector<std::complex<double>> _curr_coeffs;
    std::vector<std::complex<double>> _next_coeffs;
    std::size_t _n;
};

};
