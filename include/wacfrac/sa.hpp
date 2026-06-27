#pragma once

#include <wacfrac/orbit.hpp>
#include <vector>
#include <cstddef>

namespace wacfrac {

template <Complex T = std::complex<long double>>
class SeriesApproximator {
    public:
    SeriesApproximator(const std::vector<T>& reference_orbit, std::size_t coefficient_count, std::size_t n, double escape_radius = 2.0);
    SeriesApproximator(const std::vector<T>& reference_orbit, std::size_t coefficient_count, const std::vector<T>& probes, double validity_threshold = 1e-6, double escape_radius = 2.0);
    auto approximate_escape(T dc) const -> std::pair<T, std::size_t>;
    auto approximate_delta_n(T delta_0) const -> T;
    auto n() const -> std::size_t;

    private:
    SeriesApproximator(const std::vector<T>& reference_orbit, std::size_t coefficient_count, double escape_radius = 2.0);
    void resize(std::size_t coefficient_count);
    void compute_coeffs(std::size_t n);
    void compute_coeffs_while_valid(const std::vector<T>& deltas, double threshold = 1e-6);
    auto is_valid(T delta_0, double threshold = 1e-6) const -> bool;
    void compute_next_coeffs();

    const std::vector<T>& _reference_orbit;
    std::vector<T> _curr_coeffs;
    std::vector<T> _next_coeffs;
    std::size_t _n;
    double _escape_radius;
};

}; // namespace wacfrac
