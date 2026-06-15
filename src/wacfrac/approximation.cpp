// https://web.archive.org/web/20220125200420/http://www.science.eclipse.co.uk/sft_maths.pdf
#include "wacfrac/approximation.hpp"
#include <ranges>

using namespace wacfrac;

series_approximator::series_approximator(const std::vector<std::complex<double>>& reference_orbit, std::size_t coefficient_count)
    : _reference_orbit(reference_orbit)
    , _curr_coeffs(coefficient_count, {0.0, 0.0})
    , _next_coeffs(coefficient_count)
{
    if (coefficient_count > 0)
        _curr_coeffs[0] = {1.0, 0.0};
}
void series_approximator::resize(std::size_t coefficient_count) {
    _n = 0;
    _curr_coeffs.resize(coefficient_count);
    _next_coeffs.resize(coefficient_count);
    if (coefficient_count > 0) {
        std::ranges::fill(_curr_coeffs, {0.0, 0.0});
        _curr_coeffs[0] = {1.0, 0.0};
    }
}
void series_approximator::compute_next_coeffs() {
    for (auto&& c : std::views::iota(0uz, _next_coeffs.size())) {
        _next_coeffs[c] = 2.0 * _reference_orbit[_n] * _curr_coeffs[c];
        if (c == 0) {
            _next_coeffs[c] += 1.0;
            continue;
        }
        for (auto&& c1 : std::views::iota(0uz, c)) {
            auto c2 = c - c1 - 1;
            _next_coeffs[c] += (c1 != c2 ? 2.0 : 1.0) * _curr_coeffs[c1] * _curr_coeffs[c2];
        }
    }
}

void series_approximator::compute_coeffs(std::size_t n) {
    for (auto i {0uz}; i < n && _n < _reference_orbit.size(); ++i) {
        compute_next_coeffs();
        std::swap(_curr_coeffs, _next_coeffs);
        ++_n;
    }
    
}
void series_approximator::compute_coeffs_while_valid(const std::vector<std::complex<double>>& deltas, double threshold) {
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

auto series_approximator::approximate_delta_n(std::complex<double> delta_0) const -> std::complex<double> {
    std::complex<double> z_n {0.0, 0.0};
    std::complex<double> c_pow {1.0, 0.0};
    for (auto& coeff : _curr_coeffs) {
        c_pow *= delta_0;
        z_n += coeff * c_pow;
    }
    return z_n;
}
auto series_approximator::is_valid(std::complex<double> delta_0, double threshold) const -> bool {
    std::complex<double> c_pow {1.0, 0.0};
    std::complex<double> prev_term {0.0, 0.0};
    for (std::size_t i = 0; i < _curr_coeffs.size(); ++i) {
        c_pow *= delta_0;
        auto term = _curr_coeffs[i] * c_pow;
        if (i > 0 && std::abs(term) > threshold * std::abs(prev_term))
            return false;
        prev_term = term;
    }
    return true;
}
auto series_approximator::n() const -> std::size_t {
    return _n;
}