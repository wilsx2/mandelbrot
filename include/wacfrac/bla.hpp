#pragma once

#include "wacfrac/complex_concept.hpp"
#include <wacfrac/orbit.hpp>
#include <wacfrac/types.hpp>
#include <wacfrac/log.hpp>
#include <vector>
#include <optional>
#include <ranges>
#include <cmath>
#include <cstddef>

namespace wacfrac {

template <ComplexConcept T = SingleComplex>
class BivariateLinearApproximator {
    public:
    using CT = ComplexValueTypeT<T>;
    struct ApproximationParams {
        CT epsilon;
        std::span<const T> ref;
        T max_dc;
    };
    struct SearchParams {
        double lower_exp;
        double upper_exp;
        double tolerance;
        double convergence_radius = 1e-3;
    };
    BivariateLinearApproximator(std::size_t max_n, std::size_t first_level); 
    auto compute_manual(CT epsilon, std::span<const T> ref, T max_dc) -> bool;
    auto compute_search(SearchParams params, const std::vector<T>& probes, T max_dc, const std::vector<T>& ref, double escape_radius = 2.0) -> bool;

    auto approximate_dzn(T dzm, unsigned m, T dc) const -> std::optional<std::pair<T, unsigned>>;

    private:
    struct Approximation {
        T a, b;
        CT r;
        Approximation() = default;
        Approximation(T a, T b, CT r) : a(a), b(b), r(r) {}
        Approximation(const ApproximationParams& params, unsigned m, unsigned n);
        auto is_valid(T dzm) const -> bool;
        auto approximate_dzn(T dzm, T dc) const -> T;
        static auto merge(const T& max_dc, const Approximation& x, const Approximation& y) -> Approximation;
    };

    struct ColumnInfo {
        std::size_t first;
        std::size_t count;
    };
    auto approximation_exists(unsigned m, std::size_t level) const -> bool;
    auto approximation_at(unsigned m, std::size_t level) const -> const Approximation*;
    auto approximation_at(unsigned m, std::size_t level) -> Approximation*;

    std::size_t _max_ref_size;
    std::size_t _first_level;
    std::size_t _last_level;
    std::vector<Approximation> _working_approximations;
    std::vector<Approximation> _approximations;
    std::vector<ColumnInfo> _columns;
};

template <ComplexConcept T>
BivariateLinearApproximator<T>::BivariateLinearApproximator(std::size_t max_n, std::size_t first_level)
    : _max_ref_size(max_n + 1)
    , _first_level(first_level)
    , _last_level(max_n < 2 ? std::size_t{0} : static_cast<std::size_t>(std::log2(static_cast<double>(max_n + 1))))
    , _working_approximations(max_n - 1)
    // TODO: Allocate approximations
    // TODO: Allocate columns
    
{
    _columns.assign(max_n - 1, {0, 0});
    auto i {0uz};
    for (auto m : std::views::iota(1uz, max_n)) {
        auto cz {static_cast<unsigned>(std::countr_zero(m - 1))};
        auto size {cz >= _first_level
            ? 1 + std::min(cz - _first_level, _last_level - _first_level)
            : 0uz};
        _columns.at(m - 1) = {i, size};
        i += size;
    }
    _approximations.resize(i); // TODO: Remove this allocation, see initializers
}

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::compute_manual(CT epsilon, std::span<const T> ref, T max_dc) -> bool {
    if (ref.size() > _max_ref_size) {
        logging::error("Reference size {} exceeds maximum reference size {} for this approximator", ref.size(), _max_ref_size);
        return false;
    }

    auto level_size {ref.size() - 2};
    for (auto m : std::views::iota(1uz, ref.size() - 1)) {
        Approximation bla {{epsilon, ref, max_dc}, static_cast<unsigned>(m), static_cast<unsigned>(m + 1)};
        _working_approximations.at(m - 1) = bla;
        if (0 == _first_level) {
            *approximation_at(m, 0) = bla;
        }
    }

    for (auto i {1uz}; level_size >= 2; ++i) {
        auto even_size {level_size & ~1uz};
        for (auto k : std::views::iota(0uz, even_size) | std::views::stride(2)) {
            auto bla {Approximation::merge(max_dc, _working_approximations.at(k), _working_approximations.at(k+1))};
            _working_approximations.at(k/2) = bla;
            if (i >= _first_level) {
                auto m {1 + (k / 2) * (1uz << i)};
                *approximation_at(m, i) = bla;
            }
        }
        level_size /= 2;
    }

    return true;
}

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::compute_search(SearchParams params, const std::vector<T>& probes, T max_dc, const std::vector<T>& ref, double escape_radius) -> bool
{
    if (ref.size() > _max_ref_size) {
        logging::error("Reference size {} exceeds maximum reference size {} for this approximator", ref.size(), _max_ref_size);
        return false;
    }

    logging::info( "Searching for optimal BLA epsilon: tolerance={} probes={} range=10^[{}, {}]", params.tolerance, probes.size(), params.lower_exp, params.upper_exp);

    std::vector<unsigned> true_escape_times;
    true_escape_times.reserve(probes.size());
    std::ranges::transform(probes, std::back_inserter(true_escape_times),
        [&ref, &escape_radius](T p) -> unsigned { return escape_perturbed<T>(p, ref, static_cast<unsigned>(ref.size()), escape_radius).second; });

    auto prev_avg_skipped {-1.0};
    BivariateLinearApproximator<T> prev_bla {_max_ref_size, _first_level};
    auto lower_exp {params.lower_exp};
    auto upper_exp {params.upper_exp};
    constexpr auto UPPER_LIMIT {32uz};
    for (auto iter : std::views::iota(0uz, UPPER_LIMIT)) {
        auto middle {(upper_exp + lower_exp) / 2.0};

        auto epsilon = static_cast<ComplexValueTypeT<T>>(std::pow(10.0, middle));
        (void) compute_manual(epsilon, ref, max_dc);

        if (upper_exp - lower_exp < params.convergence_radius) {
            logging::trace( "BLA search iter {}: epsilon=10^{} (converged)", iter, middle);
            break;
        }

        auto all_correct{true};
        auto total_skipped{0u};
        for (auto&& [i, probe] : probes | std::views::enumerate) {
            auto [_, approx_escape_time, skipped] = escape_approximate(probe, std::span<const T>(ref), static_cast<unsigned>(ref.size()), escape_radius, *this);

            using std::abs;
            if (abs(static_cast<double>(approx_escape_time) / static_cast<double>(true_escape_times.at(i)) - 1.0) > params.tolerance) {
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

    return true;
}

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::approximate_dzn(T dzm, unsigned m, T dc) const -> std::optional<std::pair<T, unsigned>> {
    auto bla = approximation_at(m, _first_level);
    if (!bla || !bla->is_valid(dzm))
        return std::nullopt;

    auto n = m + 1;
    for (auto level : std::views::iota(_first_level, _last_level + 1) | std::views::reverse) {
        auto next_bla = approximation_at(m, level);
        if (next_bla && next_bla->is_valid(dzm)) {
            bla = next_bla;
            n = m + (1u << level);
            break;
        }
    }

    return {{bla->approximate_dzn(dzm, dc), n}};
}

template <ComplexConcept T>
BivariateLinearApproximator<T>::Approximation::Approximation(const ApproximationParams& params, unsigned m, unsigned n) {
    using std::abs;
    auto l {n - m};
    a = T{2.0, 0.0} * params.ref[m] * static_cast<CT>(l);
    b = T{static_cast<CT>(l), 0.0};
    auto denom = abs(a);
    r = denom > CT{}
        ? (params.epsilon * abs(params.ref[n]) - abs(b) * abs(params.max_dc)) / denom
        : -abs(params.max_dc);
}

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::Approximation::is_valid(T dzm) const -> bool {
    using std::norm;
    return r > CT{} && norm(dzm) < r*r;
}

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::Approximation::approximate_dzn(T dzm, T dc) const -> T {
    return a*dzm + b*dc;
}

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::Approximation::merge(const T& max_dc, const Approximation& x, const Approximation& y) -> Approximation {
    using std::abs;
    auto a {x.a * y.a};
    auto b {y.a * x.b + y.b};
    auto denom = abs(a);
    auto r = denom > CT{}
        ? std::min(x.r, (y.r - abs(b) * abs(max_dc)) / denom)
        : std::min(x.r, y.r - abs(b) * abs(max_dc));
    return {a, b, r};
}

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::approximation_exists(unsigned m, std::size_t level) const -> bool {
    return level >= _first_level && m > 0 && m - 1 < _columns.size() && level - _first_level < _columns.at(m - 1).count;
}

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::approximation_at(unsigned m, std::size_t level) const -> const Approximation* {
    if (approximation_exists(m, level))
        return &_approximations.at(_columns.at(m - 1).first + level - _first_level);
    return nullptr;
}

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::approximation_at(unsigned m, std::size_t level) -> Approximation* {
    if (approximation_exists(m, level))
        return &_approximations.at(_columns.at(m - 1).first + level - _first_level);
    return nullptr;
}

template <ComplexConcept T>
auto escape_approximate(const T& dc, std::span<const T> ref, unsigned max_n, double escape_radius, BivariateLinearApproximator<T> approximator) -> std::tuple<Complex<float>, unsigned, unsigned> {
    unsigned ref_n {0u};
    unsigned skipped {0u};
    T dz {0.0, 0.0};
    auto [z, n] = escape_generic(T{}, max_n, escape_radius,
        [&](T& z, unsigned& n){
            auto approximation {approximator.approximate_dzn(dz, ref_n, dc)};
            if (approximation) {
                auto m {ref_n};
                std::tie(dz, ref_n) = *approximation;
                skipped += ref_n - m;
                n += ref_n - m;
            } else {
                compute_next_perturbation<T>(z, dz, n, dc, ref, ref_n);
            }
            rebase_perturbation<T>(z, dz, ref, ref_n);
        });
    return {z, n, skipped};
}


} // namespace wacfrac
