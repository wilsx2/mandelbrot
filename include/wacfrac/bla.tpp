#pragma once

#include <wacfrac/bla.hpp>
#include <wacfrac/log.hpp>
#include <wacfrac/orbit.hpp>
#include <cstddef>
#include <optional>
#include <ranges>
#include <cmath>

namespace wacfrac {

template <Complex T>
bivariate_linear_approximator<T>::bivariate_linear_approximator(const std::vector<T>& ref, std::size_t first_level)
    : _ref(ref)
    , _first_level(first_level)
    , _last_level(std::log2(_ref.size()))
    , _columns(ref.size() - 2)
{
    auto i {0uz};
    for (auto m : std::views::iota(1uz, ref.size() - 1)) {
        auto cz {static_cast<std::size_t>(std::countr_zero(m - 1))};
        auto size {cz >= _first_level
            ? 1 + std::min(cz - _first_level, _last_level - _first_level)
            : 0uz};
        _columns.at(m - 1) = {i, size};
        i += size;
    }
    _blas.resize(i);
}

template <Complex T>
bivariate_linear_approximator<T>::bivariate_linear_approximator(double epsilon, T max_dc, const std::vector<T>& ref, std::size_t first_level)
    : bivariate_linear_approximator(ref, first_level)
{
    logging::print(logging::severity::debug, "Computing BLA with epsilon={} first_level={} ref.size={}", epsilon, first_level, ref.size());
    using Real = complex_value_type_t<T>;
    compute_blas(Real{epsilon}, max_dc);
    logging::print(logging::severity::debug, "BLA computed: {} coefficients across {} columns, levels {}-{}", _blas.size(), _columns.size(), _first_level, _last_level);
}

template <Complex T>
bivariate_linear_approximator<T>::bivariate_linear_approximator(double tolerance, const std::vector<T>& probes, T max_dc, const std::vector<T>& ref, std::size_t first_level)
    : bivariate_linear_approximator(ref, first_level)
{
    logging::print(logging::severity::info, "Searching for optimal BLA epsilon: tolerance={} probes={}", tolerance, probes.size());

    std::vector<std::size_t> true_escape_times;
    true_escape_times.reserve(probes.size());
    std::ranges::transform(probes, std::back_inserter(true_escape_times),
        [&ref](T p) -> std::size_t { return escape_perturbed<T>(ref, p, ref.size()).second; });

    (void)first_level;
    using Real = complex_value_type_t<T>;

    // Find max |ref[n]| for epsilon bound computation
    Real max_ref_abs{0};
    for (auto& r : ref) {
        auto ra = abs(r);
        if (ra > max_ref_abs) max_ref_abs = ra;
    }

    auto max_dc_abs = abs(max_dc);

    auto epsilon_base = max_ref_abs > Real{0} ? max_dc_abs / max_ref_abs : max_dc_abs;
    if (epsilon_base == Real{0}) {
        epsilon_base = Real{std::numeric_limits<double>::min()};
    }

    double log10_base;
    if constexpr (std::is_floating_point_v<Real>) {
        log10_base = static_cast<double>(std::log10(epsilon_base));
    } else {
        log10_base = static_cast<double>(boost::multiprecision::log10(epsilon_base));
    }
    auto lower = log10_base - 3.0;
    auto upper = log10_base + std::log10(static_cast<double>(ref.size())) + 6.0;
    if (upper <= lower) {
        upper = lower + 9.0;
    }

    logging::print(logging::severity::trace, "BLA epsilon search: log10_base={} range=[{}, {}]", log10_base, lower, upper);

    auto prev_avg_skipped{-1.0};
    constexpr auto upper_limit{32uz};
    for (auto iter{0uz}; iter < upper_limit; ++iter) {
        auto middle{(upper + lower) / 2.0};
        auto offset = middle - log10_base;
        auto epsilon = epsilon_base * Real{std::pow(10.0, offset)};
        compute_blas(epsilon, max_dc);

        auto all_correct{true};
        auto total_skipped{0uz};
        for (auto&& [i, probe] : probes | std::views::enumerate) {
            auto [_, approx_escape_time, skipped] = escape_approximate(probe);

            using std::abs;
            if (abs(static_cast<double>(approx_escape_time) / static_cast<double>(true_escape_times.at(i)) - 1.0) > tolerance) {
                all_correct = false;
                break;
            }
            total_skipped += skipped;
        }

        if (!all_correct) {
            logging::print(logging::severity::trace, "BLA search iter {}: epsilon=10^{} too high", iter, middle);
            upper = middle;
            continue;
        }

        auto avg_skipped = total_skipped / static_cast<double>(probes.size());
        if (avg_skipped > prev_avg_skipped) {
            logging::print(logging::severity::trace, "BLA search iter {}: epsilon=10^{} avg_skipped={} (improving)", iter, middle, avg_skipped);
            prev_avg_skipped = avg_skipped;
            lower = middle;
        } else {
            logging::print(logging::severity::trace, "BLA search iter {}: epsilon=10^{} avg_skipped={} (converged)", iter, middle, avg_skipped);
            // Recompute BLAs with the previous (better) epsilon
            auto best_offset = lower - log10_base;
            auto best_epsilon = epsilon_base * Real{std::pow(10.0, best_offset)};
            compute_blas(best_epsilon, max_dc);
            break;
        }
    }
    logging::print(logging::severity::info, "BLA epsilon search complete");
}

template <Complex T>
auto bivariate_linear_approximator<T>::escape_approximate(T dc) const -> std::tuple<T, std::size_t, std::size_t> {
    auto ref_n {0uz};
    auto n {0uz};
    auto skipped {0uz};
    T dz {0.0, 0.0};
    T z {0.0, 0.0};
    while (n < _ref.size() && !escaped(z)) {
        auto approximation = compute_zn(dc, dz, ref_n);
        if (approximation) {
            auto m {ref_n};
            std::tie(dz, ref_n) = *approximation;
            skipped += ref_n - m;
            n += ref_n - m;
            std::tie(ref_n, dz, z) = rebase_reference<T>(_ref, ref_n, dz);
        } else {
            std::tie(ref_n, dz, z) = compute_next_perturbation<T>(_ref, ref_n, dc, dz);
            ++n;
        }
    }
    return std::make_tuple(dz, n, skipped);
}

template <Complex T>
auto bivariate_linear_approximator<T>::compute_zn(T dc, T dzm, std::size_t m) const -> std::optional<std::pair<T, std::size_t>> {
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

    if (n >= _ref.size())
        return std::nullopt;

    return {{bla->approximate_dzn(dzm, dc), n}};
}

template <Complex T>
auto bivariate_linear_approximator<T>::compute_bla(complex_value_type_t<T> epsilon, T max_dc, std::size_t m, std::size_t n) const -> bla {
    using Real = complex_value_type_t<T>;
    using std::abs;
    auto l {n - m};
    auto a = T{2.0, 0.0} * _ref.at(m) * static_cast<Real>(l);
    auto b = T{static_cast<Real>(l), 0.0};
    auto denom = abs(a);
    auto r = denom > Real{}
        ? (epsilon * abs(_ref[n]) - abs(b) * abs(max_dc)) / denom
        : -abs(max_dc);
    return {a, b, r};
}

template <Complex T>
auto bivariate_linear_approximator<T>::compute_blas(complex_value_type_t<T> epsilon, T max_dc) -> void {
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

template <Complex T>
auto bivariate_linear_approximator<T>::merge_blas(T max_dc, const bla& x, const bla& y) const -> bla {
    using Real = complex_value_type_t<T>;
    using std::abs;
    auto a {x.a * y.a};
    auto b {y.a * x.b + y.b};
    auto denom = abs(a);
    auto r = denom > Real{}
        ? std::min(x.r, (y.r - abs(b) * abs(max_dc)) / denom)
        : std::min(x.r, y.r - abs(b) * abs(max_dc));
    return {a, b, r};
}

template <Complex T>
auto bivariate_linear_approximator<T>::bla_exists(std::size_t m, std::size_t level) const -> bool {
    return level >= _first_level && m > 0 && m - 1 < _columns.size() && level - _first_level < _columns.at(m - 1).count;
}

template <Complex T>
auto bivariate_linear_approximator<T>::bla_at(std::size_t m, std::size_t level) const -> const bla* {
    if (bla_exists(m, level))
        return &_blas.at(_columns.at(m - 1).first + level - _first_level);
    return nullptr;
}

template <Complex T>
auto bivariate_linear_approximator<T>::bla_at(std::size_t m, std::size_t level) -> bla* {
    if (bla_exists(m, level))
        return &_blas.at(_columns.at(m - 1).first + level - _first_level);
    return nullptr;
}

} // namespace wacfrac
