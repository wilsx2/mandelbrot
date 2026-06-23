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
        auto size {1 + std::min(static_cast<std::size_t>(std::countr_zero(m - 1)), _last_level - _first_level)};
        _columns.at(m - 1) = {i, size};
        i += size;
    }
    _blas.resize(i);
}

template <Complex T>
bivariate_linear_approximator<T>::bivariate_linear_approximator(double epsilon, T max_dc, const std::vector<T>& ref, std::size_t first_level)
    : bivariate_linear_approximator(ref, first_level)
{
    LOG_DEBUG << "Computing BLA with epsilon=" << epsilon
              << " first_level=" << first_level
              << " ref.size=" << ref.size();
    compute_blas(epsilon, max_dc);
    LOG_DEBUG << "BLA computed: " << _blas.size() << " coefficients across "
              << _columns.size() << " columns, levels " << _first_level
              << "-" << _last_level;
}

template <Complex T>
bivariate_linear_approximator<T>::bivariate_linear_approximator(double initial_epsilon, double tolerance, const std::vector<T>& probes, T max_dc, const std::vector<T>& ref, std::size_t first_level)
    : bivariate_linear_approximator(ref, first_level)
{
    LOG_INFO << "Searching for optimal BLA epsilon: initial_epsilon=" << initial_epsilon
             << " tolerance=" << tolerance << " probes=" << probes.size();

    std::vector<std::size_t> true_escape_times;
    true_escape_times.reserve(probes.size());
    std::ranges::transform(probes, std::back_inserter(true_escape_times),
        [&ref](T p) -> std::size_t { return escape_perturbed<T>(ref, p, ref.size()).second; });

    auto exponent {std::floor(std::log10(initial_epsilon))};
    auto coeff {initial_epsilon / std::pow(10, exponent)};
    auto upper {coeff * std::pow(10.0, exponent * 2.0)};
    auto lower {coeff * std::pow(10.0, exponent / 2.0)};
    if (upper < lower) std::swap(upper, lower);
    auto prev_avg_skipped {0.0};
    constexpr auto upper_limit {100uz};
    for (auto iter {0uz}; iter < upper_limit; ++iter) {
        auto epsilon = (upper + lower) / 2.0;
        compute_blas(epsilon, max_dc);

        auto all_correct {true};
        auto total_skipped {0uz};
        for (auto&& [i, probe] : probes | std::views::enumerate) {
            auto [_, approx_escape_time, skipped] = escape_approximate(probe);

            using std::abs;
            if (abs(approx_escape_time / static_cast<double>(true_escape_times.at(i)) - 1.0) > tolerance) {
                all_correct = false;
                break;
            }

            total_skipped += skipped;
        }
        if (!all_correct) {
            LOG_TRACE << "BLA epsilon search iter " << iter << ": epsilon=" << epsilon
                      << " too high (probe escape mismatch)";
            upper = epsilon;
            break;
        }

        auto avg_skipped {total_skipped / static_cast<double>(probes.size())};
        if (avg_skipped > prev_avg_skipped) {
            LOG_TRACE << "BLA epsilon search iter " << iter << ": epsilon=" << epsilon
                      << " avg_skipped=" << avg_skipped << " (improving)";
            prev_avg_skipped = avg_skipped;
            lower = epsilon;
        } else {
            LOG_TRACE << "BLA epsilon search iter " << iter << ": epsilon=" << epsilon
                      << " avg_skipped=" << avg_skipped << " (converged)";
            break;
        }
    }
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

    return {{bla->approximate_dzn(dzm, dc), n}};
}

template <Complex T>
auto bivariate_linear_approximator<T>::compute_bla(double epsilon, T max_dc, std::size_t m, std::size_t n) const -> bla {
    using Real = complex_value_type_t<T>;
    using std::abs;
    auto l {n - m};
    auto a = T{2.0, 0.0} * _ref.at(m) * static_cast<Real>(l);
    auto b = T{static_cast<Real>(l), 0.0};
    auto denom = abs(a);
    auto r = denom > Real{}
        ? (Real{epsilon} * abs(_ref[n]) - abs(b) * abs(max_dc)) / denom
        : -abs(max_dc);
    return {a, b, r};
}

template <Complex T>
auto bivariate_linear_approximator<T>::compute_blas(double epsilon, T max_dc) -> void {
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
