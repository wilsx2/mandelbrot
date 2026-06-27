#pragma once

#include <wacfrac/log.hpp>
#include <wacfrac/sa.hpp>
#include <ranges>

namespace wacfrac {

template <Complex T>
SeriesApproximator<T>::SeriesApproximator(const std::vector<T>& reference_orbit, std::size_t coefficient_count, double escape_radius)
    : _reference_orbit(reference_orbit)
    , _curr_coeffs(coefficient_count, T{0.0, 0.0})
    , _next_coeffs(coefficient_count)
    , _n(1)
    , _escape_radius(escape_radius)
{
    if (coefficient_count > 0)
        _curr_coeffs[0] = T{1.0, 0.0};
}
template <Complex T>
SeriesApproximator<T>::SeriesApproximator(const std::vector<T>& reference_orbit, std::size_t coefficient_count, std::size_t n, double escape_radius)
    : SeriesApproximator(reference_orbit, coefficient_count, escape_radius)
{
    logging::print(logging::Severity::Debug, "Computing SA coefficients: count={} n={} ref.size={}", coefficient_count, n, reference_orbit.size());
    compute_coeffs(n);
    logging::print(logging::Severity::Debug, "SA computed: {} steps, {} coefficients", _n, _curr_coeffs.size());
}

template <Complex T>
SeriesApproximator<T>::SeriesApproximator(const std::vector<T>& reference_orbit, std::size_t coefficient_count, const std::vector<T>& probes, double validity_threshold, double escape_radius)
    : SeriesApproximator(reference_orbit, coefficient_count, escape_radius)
{
    logging::print(logging::Severity::Debug, "Computing SA coefficients (validity-driven): count={} threshold={} probes={}", coefficient_count, validity_threshold, probes.size());
    compute_coeffs_while_valid(probes, validity_threshold);
    logging::print(logging::Severity::Debug, "SA computed: {} steps, {} coefficients", _n, _curr_coeffs.size());
}

template <Complex T>
auto SeriesApproximator<T>::approximate_escape(T dc) const -> std::pair<T, std::size_t> {
    auto dz = this->approximate_delta_n(dc);
    return escape_perturbed<T>(_reference_orbit, dc, _reference_orbit.size(), _escape_radius, dz, _n);
}

template <Complex T>
void SeriesApproximator<T>::resize(std::size_t coefficient_count) {
    _n = 1;
    _curr_coeffs.resize(coefficient_count);
    _next_coeffs.resize(coefficient_count);
    if (coefficient_count > 0) {
        std::ranges::fill(_curr_coeffs, T{0.0, 0.0});
        _curr_coeffs[0] = T{1.0, 0.0};
    }
}

template <Complex T>
void SeriesApproximator<T>::compute_next_coeffs() {
    for (auto&& c : std::views::iota(0uz, _next_coeffs.size())) {
        _next_coeffs[c] = T{2.0, 0.0} * _reference_orbit[_n] * _curr_coeffs[c];
        if (c == 0) {
            _next_coeffs[c] += T{1.0, 0.0};
            continue;
        }
        for (auto&& c1 : std::views::iota(0uz, c)) {
            auto c2 = c - c1 - 1;
            _next_coeffs[c] += (c1 != c2 ? T{2.0, 0.0} : T{1.0, 0.0}) * _curr_coeffs[c1] * _curr_coeffs[c2];
        }
    }
}

template <Complex T>
void SeriesApproximator<T>::compute_coeffs(std::size_t n) {
    for (auto i {0uz}; i < n && _n < _reference_orbit.size(); ++i) {
        compute_next_coeffs();
        std::swap(_curr_coeffs, _next_coeffs);
        ++_n;
    }
}

template <Complex T>
void SeriesApproximator<T>::compute_coeffs_while_valid(const std::vector<T>& deltas, double threshold) {
    while (_n < _reference_orbit.size()) {
        compute_next_coeffs();
        std::swap(_curr_coeffs, _next_coeffs);

        bool valid {true};
        for (auto delta : deltas) {
            if (!is_valid(delta, threshold)) {
                valid = false;
                break;
            }
        }

        if (valid) {
            ++_n;
        } else {
            std::swap(_curr_coeffs, _next_coeffs);
            break;
        }
    }
}

template <Complex T>
auto SeriesApproximator<T>::approximate_delta_n(T delta_0) const -> T {
    T z_n {0.0, 0.0};
    T c_pow {1.0, 0.0};
    for (auto& coeff : _curr_coeffs) {
        c_pow *= delta_0;
        z_n += coeff * c_pow;
    }
    return z_n;
}

template <Complex T>
auto SeriesApproximator<T>::is_valid(T delta_0, double threshold) const -> bool {
    T c_pow {1.0, 0.0};
    T prev_term {0.0, 0.0};
    for (std::size_t i = 0; i < _curr_coeffs.size(); ++i) {
        c_pow *= delta_0;
        T term = _curr_coeffs[i] * c_pow;
        if (i > 0) {
            using std::abs;
            if (abs(term) > static_cast<ComplexValueTypeT<T>>(threshold) * abs(prev_term))
                return false;
        }
        prev_term = term;
    }
    return true;
}

template <Complex T>
auto SeriesApproximator<T>::n() const -> std::size_t {
    return _n;
}

} // namespace wacfrac
