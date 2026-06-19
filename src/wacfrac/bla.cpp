#include "wacfrac/bla.hpp"
#include "wacfrac/orbit.hpp"
#include <cstddef>
#include <optional>
#include <ranges>

using namespace wacfrac;

// https://philthompson.me/2023/Faster-Mandelbrot-Set-Rendering-with-BLA-Bivariate-Linear-Approximation.html
bivariate_linear_approximator::bivariate_linear_approximator(const std::vector<std::complex<double>>& ref, std::size_t first_level)
    : _ref(ref)
    , _first_level(first_level)
    , _last_level(std::log2(_ref.size()))
    , _columns(ref.size() - 2)
{
    auto i {0uz};
    for (auto m : std::views::iota(1uz, ref.size() - 1)) {
        auto size {1 + std::min(static_cast<std::size_t>(std::countr_zero(m - 1)), _last_level - _first_level)};
        _columns.at(m - 1) = {i, size};
        i += size;
    }
    _blas.resize(i);
}
bivariate_linear_approximator::bivariate_linear_approximator(double epsilon, std::complex<double> max_dc, const std::vector<std::complex<double>>& ref,  std::size_t first_level)
    : bivariate_linear_approximator(ref, first_level)
{
    compute_blas(epsilon, max_dc);
}

bivariate_linear_approximator::bivariate_linear_approximator(double initial_epsilon, double tolerance, const std::vector<std::complex<double>>& probes, std::complex<double> max_dc, const std::vector<std::complex<double>>& ref, std::size_t first_level)
    : bivariate_linear_approximator(ref, first_level)
{
    std::vector<std::size_t> true_escape_times;
    true_escape_times.reserve(probes.size());
    std::ranges::transform(probes, std::back_inserter(true_escape_times),
        [&ref](std::complex<double> p) -> std::size_t { return escape_perturbed(ref, p, ref.size()).second; });
    
    // Perform binary search for ideal epsilon
    auto exponent {std::floor(std::log10(initial_epsilon))};
    auto coeff {initial_epsilon / std::pow(10, exponent)};
    auto upper {coeff * std::pow(10.0, exponent * 2.0)};
    auto lower {coeff * std::pow(10.0, exponent / 2.0)};
    auto found_heuristic {false};
    auto prev_avg_skipped {0.0};
    while (!found_heuristic) {
        auto epsilon = (upper + lower) / 2.0;
        compute_blas(epsilon, max_dc);

        auto all_correct {true};
        auto total_skipped {0uz};
        for (auto&& [i, probe] : probes | std::views::enumerate) {
            auto [_, approx_escape_time, skipped] = escape_approximate(probe);
           
            if (std::abs(approx_escape_time / static_cast<double>(true_escape_times.at(i)) - 1.0) > tolerance) {
                all_correct = false;
                break;
            }
            
            total_skipped += skipped;
        }
        if (!all_correct) {
            upper = epsilon;
            break;
        }
        
        auto avg_skipped {total_skipped / static_cast<double>(probes.size())};
        if (avg_skipped > prev_avg_skipped) {
            lower = epsilon;
        } else {
            found_heuristic = true;
        }
    }
}

auto bivariate_linear_approximator::escape_approximate(std::complex<double> dc) const -> std::tuple<std::complex<double>, std::size_t, std::size_t> {
    auto ref_n {0uz};
    auto n {0u};
    auto skipped {0uz};
    std::complex<double> dz {0.0, 0.0};
    std::complex<double> z {0.0, 0.0};
    while (n < _ref.size() && !escaped(z)) {
        auto approximation = compute_zn(dc, dz, ref_n);
        if (approximation) {
            auto m {ref_n};
            std::tie(dz, ref_n) = *approximation;
            n += ref_n - m;
            std::tie(ref_n, dz, z) = rebase_reference(_ref, ref_n, dz);
        } else {
            std::tie(ref_n, dz, z) = compute_next_perturbation(_ref, ref_n, dc, dz);
            ++n;
        }
    }
    return std::make_tuple(dz, n, skipped);
}

auto bivariate_linear_approximator::compute_zn(std::complex<double> dc, std::complex<double> dzm, std::size_t m) const -> std::optional<std::pair<std::complex<double>, std::size_t>> {
    auto bla = bla_at(m, _first_level);
    if (!bla || !bla->is_valid(dzm))
        return std::nullopt;

    auto n = m + 1;
    for (auto level : std::views::iota(_first_level, _last_level + 1) | std::views::reverse) {
        auto next_bla = bla_at(m, level);
        if (next_bla && next_bla->is_valid(dzm)) {
            bla = next_bla;
            n = m + (1 << level);
            break;
        }
    }

    return {{bla->approximate_dzn(dzm, dc), n}};
}

auto bivariate_linear_approximator::bla::is_valid(std::complex<double> dzm) const -> bool {
    return r > 0.0 && std::norm(dzm) < r*r;
}
auto bivariate_linear_approximator::bla::approximate_dzn(std::complex<double> dzm, std::complex<double> dc) const -> std::complex<double> {
    return a*dzm + b*dc;
}
auto bivariate_linear_approximator::compute_bla(double epsilon, std::complex<double> max_dc, std::size_t m, std::size_t n) const -> bla {
    auto l {n - m};
    auto a {2.0 * _ref.at(m) * static_cast<double>(l)};
    auto b {static_cast<std::complex<double>>(1.0 * l)};
    auto denom = std::abs(a);
    auto r = denom > 0.0
        ? (epsilon * std::abs(_ref[n]) - std::abs(b) * std::abs(max_dc)) / denom
        : -std::abs(max_dc);
    return {a,b,r};
}

auto bivariate_linear_approximator::compute_blas(double epsilon, std::complex<double> max_dc) -> void {
    std::vector<bla> current_level (_ref.size() - 2);
    for (auto m : std::views::iota(1uz, _ref.size() - 1)) {
        auto bla = compute_bla(epsilon, max_dc, m, m + 1);
        current_level.at(m - 1) = bla;
        if (0 == _first_level) {
            *bla_at(m, 0) = bla;
        }
    }

    for (auto i {1uz}; current_level.size() >= 2; ++i) {
        auto even_size {current_level.size() & ~1uz};
        for (auto k : std::views::iota(0uz, even_size) | std::views::stride(2)) {
            auto bla = merge_blas(max_dc, current_level.at(k), current_level.at(k+1));
            current_level.at(k/2) = bla;
                if (i >= _first_level) {
                    auto m {1 + (k / 2) * (1uz << i)};
                    *bla_at(m, i) = bla;
                }
        }
        current_level.resize(current_level.size()/2);
    }
    
    _blas.shrink_to_fit();
}
auto bivariate_linear_approximator::merge_blas(std::complex<double> max_dc, const bla& x, const bla& y) const -> bla {
    auto a {x.a * y.a};
    auto b {y.a * x.b + y.b};
    auto denom = std::abs(a);
    auto r = denom > 0.0
        ? std::min(x.r, (y.r - std::abs(b) * std::abs(max_dc)) / denom)
        : std::min(x.r, y.r - std::abs(b) * std::abs(max_dc));
    return {a,b,r};
}
auto bivariate_linear_approximator::bla_exists(std::size_t m, std::size_t level) const -> bool {
    return level >= _first_level && m > 0 && m - 1 < _columns.size() && level - _first_level < _columns.at(m - 1).count;
}
auto bivariate_linear_approximator::bla_at(std::size_t m, std::size_t level) const -> const bla* {
    if (bla_exists(m, level))
        return &_blas.at(_columns.at(m - 1).first + level - _first_level);
    return nullptr;
}
auto bivariate_linear_approximator::bla_at(std::size_t m, std::size_t level) -> bla* {
    if (bla_exists(m, level))
        return &_blas.at(_columns.at(m - 1).first + level - _first_level);
    return nullptr;
}
