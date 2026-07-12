#pragma once

#include "wacfrac/macros.hpp"
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

struct ColumnInfo {
    std::size_t first;
    std::size_t count;
};

template <typename Derived, ComplexConcept T = SingleComplex>
class GenericBivariateLinearApproximator {
    public:
    // TODO: replace WF_STD span with Derived::NonOwningView
    using CT = ComplexValueTypeT<T>;
    struct ApproximationParams {
        CT epsilon;
        WF_STD::span<const T> ref;
        T max_dc;
    };
    struct SearchParams {
        double lower_exp;
        double upper_exp;
        double tolerance;
        double convergence_radius = 1e-3;
    };

    GenericBivariateLinearApproximator(std::size_t max_n, std::size_t first_level);
    auto initialize() -> void;
    auto compute_manual(CT epsilon, WF_STD::span<const T> ref, T max_dc) -> bool;
    auto compute_search(SearchParams params, const std::vector<T>& probes, T max_dc, WF_STD::span<const T> ref, double escape_radius = 2.0) -> bool;

    auto approximate_dzn(T dzm, unsigned m, T dc) const -> std::optional<std::pair<T, unsigned>>;
    auto resize(unsigned max_n) -> void {
        static_cast<Derived*>(this)->resize(max_n);
    }

    protected:
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

    auto approximation_exists(unsigned m, std::size_t level) const -> bool;
    auto approximation_at(unsigned m, std::size_t level) const -> const Approximation*;
    auto approximation_at(unsigned m, std::size_t level) -> Approximation*;
    auto compute_initial_approximations(const ApproximationParams& params) -> void {
        static_cast<Derived*>(this)->compute_initial_approximations(params);
    }
    auto merge_approximations(std::size_t current_level, std::size_t level_size, T max_dc) -> void {
        static_cast<Derived*>(this)->merge_approximations(current_level, level_size, max_dc);
    }
    auto compute_probe_escape_time(WF_STD::span<const T> probes, WF_STD::span<const T> ref, double escape_radius) -> WF_STD::span<const unsigned> { // TODO: replace return w/ get func
        return static_cast<Derived*>(this)->compute_probe_escape_time(probes, ref, escape_radius);
    }
    auto get_columns() -> WF_STD::span<ColumnInfo> {
        return static_cast<Derived*>(this)->get_columns();
    }
    auto get_columns() const -> WF_STD::span<const ColumnInfo> {
        return static_cast<const Derived*>(this)->get_columns();
    }
    auto get_approximations() -> WF_STD::span<Approximation> {
        return static_cast<Derived*>(this)->get_approximations();
    }
    auto get_approximations() const -> WF_STD::span<const Approximation> {
        return static_cast<const Derived*>(this)->get_approximations();
    }

    std::size_t _max_ref_size;
    std::size_t _first_level;
    std::size_t _last_level;
};

template <typename Derived, ComplexConcept T>
GenericBivariateLinearApproximator<Derived, T>::GenericBivariateLinearApproximator(std::size_t max_n, std::size_t first_level)
    : _max_ref_size(max_n + 1)
    , _first_level(first_level)
    , _last_level(max_n < 2 ? std::size_t{0} : static_cast<std::size_t>(std::log2(static_cast<double>(max_n + 1))))
{
}

template <typename Derived, ComplexConcept T>
auto GenericBivariateLinearApproximator<Derived, T>::initialize() -> void {
    auto max_n {_max_ref_size - 1};
    get_columns()[max_n - 1] = {0, 0};
    auto i {0uz};
    for (auto m : std::views::iota(1uz, max_n)) {
        auto cz {static_cast<unsigned>(std::countr_zero(m - 1))};
        auto size {cz >= _first_level
            ? 1 + std::min(cz - _first_level, _last_level - _first_level)
            : 0uz};
        get_columns()[m - 1] = {i, size};
        i += size;
    }
    resize(i);
}

template <typename Derived, ComplexConcept T>
auto GenericBivariateLinearApproximator<Derived, T>::compute_manual(CT epsilon, WF_STD::span<const T> ref, T max_dc) -> bool {
    if (ref.size() > _max_ref_size) {
        logging::error("Reference size {} exceeds maximum reference size {} for this approximator", ref.size(), _max_ref_size);
        return false;
    }

    auto level_size {ref.size() - 2};
    compute_initial_approximations({epsilon, ref, max_dc});
    for (auto i {1uz}; level_size >= 2; ++i) {
        auto even_size {level_size & ~1uz};
        merge_approximations(i, even_size, max_dc);
        level_size /= 2;
    }

    return true;
}

template <typename Derived, ComplexConcept T>
auto GenericBivariateLinearApproximator<Derived, T>::compute_search(SearchParams params, const std::vector<T>& probes, T max_dc, WF_STD::span<const T> ref, double escape_radius) -> bool
{
    if (ref.size() > _max_ref_size) {
        logging::error("Reference size {} exceeds maximum reference size {} for this approximator", ref.size(), _max_ref_size);
        return false;
    }
    logging::info( "Searching for optimal BLA epsilon: tolerance={} probes={} range=10^[{}, {}]", params.tolerance, probes.size(), params.lower_exp, params.upper_exp);

    auto true_escape_times {compute_probe_escape_time(probes, ref, escape_radius)};
    auto prev_avg_skipped {-1.0};
    CT prev_exp;
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
            auto [_, approx_escape_time, skipped] = escape_approximate(probe, WF_STD::span<const T>(ref), static_cast<unsigned>(ref.size()), escape_radius, *this);

            using std::abs;
            if (abs(static_cast<double>(approx_escape_time) / static_cast<double>(true_escape_times[i]) - 1.0) > params.tolerance) {
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
            prev_exp = epsilon;
            lower_exp = middle; 
        } else {
            logging::trace( "BLA search iter {}: epsilon=10^{} avg_skipped={} (found max)", iter, middle, avg_skipped);
            compute_manual(prev_exp, ref, max_dc);
            break;
        }
    }
    logging::info( "BLA epsilon search complete");

    return true;
}

template <typename Derived, ComplexConcept T>
auto GenericBivariateLinearApproximator<Derived, T>::approximate_dzn(T dzm, unsigned m, T dc) const -> std::optional<std::pair<T, unsigned>> {
    auto bla {approximation_at(m, _first_level)};
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

template <typename Derived, ComplexConcept T>
GenericBivariateLinearApproximator<Derived, T>::Approximation::Approximation(const ApproximationParams& params, unsigned m, unsigned n) {
    using std::abs;
    auto l {n - m};
    a = T{2.0, 0.0} * params.ref[m] * static_cast<CT>(l);
    b = T{static_cast<CT>(l), 0.0};
    auto denom = abs(a);
    r = denom > CT{}
        ? (params.epsilon * abs(params.ref[n]) - abs(b) * abs(params.max_dc)) / denom
        : -abs(params.max_dc);
}

template <typename Derived, ComplexConcept T>
auto GenericBivariateLinearApproximator<Derived, T>::Approximation::is_valid(T dzm) const -> bool {
    using std::norm;
    return r > CT{} && norm(dzm) < r*r;
}

template <typename Derived, ComplexConcept T>
auto GenericBivariateLinearApproximator<Derived, T>::Approximation::approximate_dzn(T dzm, T dc) const -> T {
    return a*dzm + b*dc;
}

template <typename Derived, ComplexConcept T>
auto GenericBivariateLinearApproximator<Derived, T>::Approximation::merge(const T& max_dc, const Approximation& x, const Approximation& y) -> Approximation {
    using std::abs;
    auto a {x.a * y.a};
    auto b {y.a * x.b + y.b};
    auto denom = abs(a);
    auto r = denom > CT{}
        ? std::min(x.r, (y.r - abs(b) * abs(max_dc)) / denom)
        : std::min(x.r, y.r - abs(b) * abs(max_dc));
    return {a, b, r};
}

template <typename Derived, ComplexConcept T>
auto GenericBivariateLinearApproximator<Derived, T>::approximation_exists(unsigned m, std::size_t level) const -> bool {
    return level >= _first_level && m > 0 && m - 1 < get_columns().size() && level - _first_level < get_columns()[m - 1].count;
}

template <typename Derived, ComplexConcept T>
auto GenericBivariateLinearApproximator<Derived, T>::approximation_at(unsigned m, std::size_t level) const -> const Approximation* {
    if (approximation_exists(m, level))
        return &get_approximations()[get_columns()[m - 1].first + level - _first_level];
    return nullptr;
}

template <typename Derived, ComplexConcept T>
auto GenericBivariateLinearApproximator<Derived, T>::approximation_at(unsigned m, std::size_t level) -> Approximation* {
    if (approximation_exists(m, level))
        return &get_approximations()[get_columns()[m - 1].first + level - _first_level];
    return nullptr;
}

template <typename Derived, ComplexConcept T>
auto escape_approximate(const T& dc, WF_STD::span<const T> ref, unsigned max_n, double escape_radius, const GenericBivariateLinearApproximator<Derived, T>& approximator) -> std::tuple<Complex<float>, unsigned, unsigned> {
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

template <typename T>
struct HostBivariateLinearApproximator;
template <typename T>
class HostBivariateLinearApproximator : public GenericBivariateLinearApproximator<HostBivariateLinearApproximator<T>, T> {
    public: 
    using Base = GenericBivariateLinearApproximator<HostBivariateLinearApproximator<T>, T>;
    using Approximation = Base::Approximation;
    using ApproximationParams = Base::ApproximationParams;
    using CT = Base::CT;

    HostBivariateLinearApproximator(std::size_t max_n, std::size_t first_level)
        : Base(max_n, first_level)
        , _columns(max_n)
        , _working_approximations(max_n - 1)
    {
        Base::initialize();
    };
    auto resize(unsigned max_n) -> void {
        _approximations.resize(max_n);
    }
    auto compute_initial_approximations(const ApproximationParams& params) -> void {
        for (auto m : std::views::iota(1uz, params.ref.size() - 1)) {
            Approximation bla {params, static_cast<unsigned>(m), static_cast<unsigned>(m + 1)};
            _working_approximations.at(m - 1) = bla;
            if (0 == Base::_first_level) {
                *(Base::approximation_at(m, 0)) = bla;
            }
        }
    }
    auto merge_approximations(std::size_t current_level, std::size_t level_size, T max_dc) -> void {
        for (auto k : std::views::iota(0uz, level_size) | std::views::stride(2)) {
            auto bla {Approximation::merge(max_dc, _working_approximations.at(k), _working_approximations.at(k+1))};
            _working_approximations.at(k/2) = bla;
            if (current_level >= Base::_first_level) {
                auto m {1 + (k / 2) * (1uz << current_level)};
                *(Base::approximation_at(m, current_level)) = bla;
            }
        }
    }
    auto compute_probe_escape_time(WF_STD::span<const T> probes, WF_STD::span<const T> ref, double escape_radius) -> WF_STD::span<const unsigned> {
        // TODO: Reduce redundant alloc
        _true_escape_times.resize(0);
        _true_escape_times.reserve(probes.size());
        std::ranges::transform(probes, std::back_inserter(_true_escape_times),
            [&ref, &escape_radius](T p) -> unsigned { return escape_perturbed<T>(p, ref, static_cast<unsigned>(ref.size()), escape_radius).second; });
        return _true_escape_times;
    }
    auto get_columns() -> WF_STD::span<ColumnInfo> {
        return _columns;
    }
    auto get_columns() const -> WF_STD::span<const ColumnInfo> {
        return _columns;
    }
    auto get_approximations() -> WF_STD::span<Approximation> {
        return _approximations;
    }
    auto get_approximations() const -> WF_STD::span<const Approximation> {
        return _approximations;
    }

    protected:
    std::vector<ColumnInfo> _columns;
    std::vector<Approximation> _working_approximations;
    std::vector<Approximation> _approximations;
    std::vector<unsigned> _true_escape_times;
};
template<typename T>
using BivariateLinearApproximator = HostBivariateLinearApproximator<T>;

} // namespace wacfrac
