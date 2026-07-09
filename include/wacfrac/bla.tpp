#pragma once

#include "wacfrac/types.hpp"
#include <wacfrac/bla.hpp>
#include <wacfrac/log.hpp>
#include <wacfrac/orbit.hpp>
#include <cstddef>
#include <optional>
#include <ranges>
#include <cmath>

namespace wacfrac {

template <ComplexConcept T>
BivariateLinearApproximator<T>::BivariateLinearApproximator(const std::vector<T>& ref, std::size_t first_level, double escape_radius)
    : _ref(ref)
    , _first_level(first_level)
    , _last_level(ref.size() < 2 ? std::size_t{0} : static_cast<std::size_t>(std::log2(static_cast<double>(ref.size()))))
    , _columns(ref.size() < 2 ? 0 : ref.size() - 2)
    , _escape_radius(escape_radius)
{
    auto i {0uz};
    for (auto m : std::views::iota(1uz, ref.size() - 1)) {
        auto cz {static_cast<unsigned>(std::countr_zero(m - 1))};
        auto size {cz >= _first_level
            ? 1 + std::min(cz - _first_level, _last_level - _first_level)
            : 0uz};
        _columns.at(m - 1) = {i, size};
        i += size;
    }
    _blas.resize(i);
}

template <ComplexConcept T>
BivariateLinearApproximator<T>::BivariateLinearApproximator(ComplexValueTypeT<T> epsilon, T max_dc, const std::vector<T>& ref, std::size_t first_level, double escape_radius)
    : BivariateLinearApproximator(ref, first_level, escape_radius)
{
    logging::debug( "Computing BLA with epsilon={} first_level={} ref.size={}", epsilon, first_level, ref.size());
    compute_blas(epsilon, max_dc);
    logging::debug( "BLA computed: {} coefficients across {} columns, levels {}-{}", _blas.size(), _columns.size(), _first_level, _last_level);
}

template <ComplexConcept T>
BivariateLinearApproximator<T>::BivariateLinearApproximator(
    double lower_exp, double upper_exp, double tolerance,
    const std::vector<T>& probes, T max_dc, const std::vector<T>& ref, std::size_t first_level, double escape_radius)
    : BivariateLinearApproximator(ref, first_level, escape_radius)
{
    logging::info( "Searching for optimal BLA epsilon: tolerance={} probes={} range=10^[{}, {}]", tolerance, probes.size(), lower_exp, upper_exp);

    std::vector<unsigned> true_escape_times;
    true_escape_times.reserve(probes.size());
    std::ranges::transform(probes, std::back_inserter(true_escape_times),
        [this, &ref](T p) -> unsigned { return escape_perturbed<T>(p, ref, static_cast<unsigned>(ref.size()), _escape_radius).second; });

    auto prev_avg_skipped {-1.0};
    BivariateLinearApproximator<T> prev_bla;
    constexpr auto UPPER_LIMIT {32uz};
    for (auto iter : std::views::iota(0uz, UPPER_LIMIT)) {
        auto middle {(upper_exp + lower_exp) / 2.0};
        
        using boost::multiprecision::pow;
        using std::pow;
        auto epsilon {pow(ComplexValueTypeT<T>{10.0}, middle)};
        compute_blas(epsilon, max_dc);

        if (upper_exp - lower_exp < 1e-3) { // TODO: Replace magic number with convergence radius
            logging::trace( "BLA search iter {}: epsilon=10^{} (converged)", iter, middle);
            break;
        }    

        auto all_correct{true};
        auto total_skipped{0u};
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
            logging::trace( "BLA search iter {}: epsilon=10^{} too high", iter, middle);
            upper_exp = middle;
            continue;
        }

        auto avg_skipped = total_skipped / static_cast<double>(probes.size());
        if (avg_skipped >= prev_avg_skipped) {
            logging::trace( "BLA search iter {}: epsilon=10^{} avg_skipped={} (improving)", iter, middle, avg_skipped);
            prev_avg_skipped = avg_skipped;
            prev_bla = *this;
            lower_exp = middle; 
        } else {
            logging::trace( "BLA search iter {}: epsilon=10^{} avg_skipped={} (found max)", iter, middle, avg_skipped);
            *this = std::move(prev_bla);
            break;
        }
    }
    logging::info( "BLA epsilon search complete");
}

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::escape_approximate(T dc) const -> std::tuple<T, unsigned, unsigned> {
    unsigned ref_n {0u};
    unsigned n {0u};
    unsigned skipped {0u};
    T dz {0.0, 0.0};
    T z {0.0, 0.0};
    while (n < _ref->get().size() && !escaped(z, _escape_radius)) {
        auto approximation = compute_zn(dc, dz, ref_n);
        if (approximation) {
            auto m {ref_n};
            std::tie(dz, ref_n) = *approximation;
            skipped += ref_n - m;
            n += ref_n - m;
        } else {
            compute_next_perturbation<T>(z, dz, n, dc, _ref->get(), ref_n);
        }
        rebase_perturbation<T>(z, dz, _ref->get(), ref_n);
    }
    return std::make_tuple(dz, n, skipped);
}

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::compute_zn(T dc, T dzm, unsigned m) const -> std::optional<std::pair<T, unsigned>> {
    auto bla = bla_at(m, _first_level);
    if (!bla || !bla->is_valid(dzm))
        return std::nullopt;

    auto n = m + 1;
    for (auto level : std::views::iota(_first_level, _last_level + 1) | std::views::reverse) {
        auto next_bla = bla_at(m, level);
        if (next_bla && next_bla->is_valid(dzm)) {
            bla = next_bla;
            n = m + (1u << level);
            break;
        }
    }

    if (n >= _ref->get().size())
        return std::nullopt;

    return {{bla->approximate_dzn(dzm, dc), n}};
}

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::compute_bla(ComplexValueTypeT<T> epsilon, T max_dc, unsigned m, unsigned n) const -> Bla {
    using Real = ComplexValueTypeT<T>;
    using std::abs;
    auto l {n - m};
    auto a = T{2.0, 0.0} * _ref->get().at(m) * static_cast<Real>(l);
    auto b = T{static_cast<Real>(l), 0.0};
    auto denom = abs(a);
    auto r = denom > Real{}
        ? (epsilon * abs(_ref->get().at(n)) - abs(b) * abs(max_dc)) / denom
        : -abs(max_dc);
    return {a, b, r};
}

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::compute_blas(ComplexValueTypeT<T> epsilon, T max_dc) -> void {
    std::vector<Bla> current_level (_ref->get().size() - 2);
    for (auto m : std::views::iota(1uz, _ref->get().size() - 1)) {
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

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::merge_blas(T max_dc, const Bla& x, const Bla& y) const -> Bla {
    using Real = ComplexValueTypeT<T>;
    using std::abs;
    auto a {x.a * y.a};
    auto b {y.a * x.b + y.b};
    auto denom = abs(a);
    auto r = denom > Real{}
        ? std::min(x.r, (y.r - abs(b) * abs(max_dc)) / denom)
        : std::min(x.r, y.r - abs(b) * abs(max_dc));
    return {a, b, r};
}

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::bla_exists(unsigned m, std::size_t level) const -> bool {
    return level >= _first_level && m > 0 && m - 1 < _columns.size() && level - _first_level < _columns.at(m - 1).count;
}

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::bla_at(unsigned m, std::size_t level) const -> const Bla* {
    if (bla_exists(m, level))
        return &_blas.at(_columns.at(m - 1).first + level - _first_level);
    return nullptr;
}

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::bla_at(unsigned m, std::size_t level) -> Bla* {
    if (bla_exists(m, level))
        return &_blas.at(_columns.at(m - 1).first + level - _first_level);
    return nullptr;
}

} // namespace wacfrac
