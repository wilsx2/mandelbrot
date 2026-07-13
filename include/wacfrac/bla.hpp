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

namespace wacfrac::bla {

struct ColumnInfo {
    std::size_t first;
    std::size_t count;
};

struct SearchParams {
    double lower_exp;
    double upper_exp;
    double tolerance;
    double convergence_radius = 1e-3;
};

template<ComplexConcept T>
struct Bla {
    using CT = ComplexValueTypeT<T>;
    T a, b;
    CT r;
    Bla() = default;
    WF_HD
    Bla(T a, T b, CT r) : a(a), b(b), r(r) {}
    template<template<typename>typename View>
    WF_HD
    Bla(CT epsilon, View<const T> ref, T max_dc, unsigned m, unsigned n) {
        using std::abs;
        auto l {n - m};
        a = T{2.0, 0.0} * ref[m] * static_cast<CT>(l);
        b = T{static_cast<CT>(l), 0.0};
        auto denom = abs(a);
        r = denom > CT{}
            ? (epsilon * abs(ref[n]) - abs(b) * abs(max_dc)) / denom
            : -abs(max_dc);
    }
    WF_HD
    auto is_valid(T dzm) const -> bool {
        using std::norm;
        return r > CT{} && norm(dzm) < r*r;
    }
    WF_HD
    auto approximate_dzn(T dzm, T dc) const -> T {
        return a*dzm + b*dc;
    }
    WF_HD
    static auto merge(const T& max_dc, const Bla& x, const Bla& y) -> Bla {
        using std::abs;
        auto a {x.a * y.a};
        auto b {y.a * x.b + y.b};
        auto denom = abs(a);
        auto r = denom > CT{}
            ? std::min(x.r, (y.r - abs(b) * abs(max_dc)) / denom)
            : std::min(x.r, y.r - abs(b) * abs(max_dc));
        return {a, b, r};
    }
};

template<template<typename>typename View, ComplexConcept T>
struct Approximator {
    using CT = ComplexValueTypeT<T>;
    std::size_t first_level;
    std::size_t last_level;
    View<const ColumnInfo> columns;
    View<Bla<T>> approximations;

    WF_HD
    auto approximation_exists(unsigned m, std::size_t level) const -> bool {
        return level >= first_level && m > 0 && m - 1 < columns.size() && level - first_level < columns[m - 1].count;
    }
    WF_HD
    auto approximation_at(unsigned m, std::size_t level) const -> const Bla<T>* {
        if (approximation_exists(m, level))
            return &approximations[columns[m - 1].first + level - first_level];
        return nullptr;
    }
    WF_HD
    auto approximation_at(unsigned m, std::size_t level) -> Bla<T>* {
        if (approximation_exists(m, level))
            return &approximations[columns[m - 1].first + level - first_level];
        return nullptr;
    }
    WF_HD
    auto approximate_dzn(T dzm, unsigned m, T dc) const -> std::optional<std::pair<T, unsigned>> {
        auto bla {approximation_at(m, first_level)};
        if (!bla || !bla->is_valid(dzm))
            return std::nullopt;

        auto n = m + 1;
        for (auto level : std::views::iota(first_level, last_level + 1) | std::views::reverse) {
            auto next_bla = approximation_at(m, level);
            if (next_bla && next_bla->is_valid(dzm)) {
                bla = next_bla;
                n = m + (1u << level);
                break;
            }
        }

        return {{bla->approximate_dzn(dzm, dc), n}};
    }
};

template <typename Derived, template<typename>typename View, ComplexConcept T>
class GenericCalculator {
    public:
    using CT = ComplexValueTypeT<T>;
    GenericCalculator(std::size_t first_level)
        : _max_ref_size(0)
        , _first_level(first_level)
        , _last_level(0)
    {}
    auto resize_for_ref(std::size_t ref_size) -> void {
        _max_ref_size = ref_size;
        _last_level = ref_size < 3 ? std::size_t{0} : static_cast<std::size_t>(std::log2(static_cast<double>(ref_size)));
        auto max_n {ref_size - 1};
        resize_columns(max_n);
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
        resize_approximations(i);
    }
    auto compute_manual(CT epsilon, View<const T> ref, T max_dc) -> Approximator<View, T> {
        if (ref.size() > _max_ref_size) {
            resize_for_ref(ref.size());
        }

        auto level_size {ref.size() - 2};
        compute_initial_approximations(epsilon, ref, max_dc);
        for (auto i {1uz}; level_size >= 2; ++i) {
            auto even_size {level_size & ~1uz};
            merge_approximations(i, even_size, max_dc);
            level_size /= 2;
        }

        return get_approximator();
    }
    auto compute_search(SearchParams params, const std::vector<T>& probes, T max_dc, View<const T> ref, double escape_radius = 2.0) -> bool {
        logging::info( "Searching for optimal BLA epsilon: tolerance={} probes={} range=10^[{}, {}]", params.tolerance, probes.size(), params.lower_exp, params.upper_exp);

        compute_probe_escape_time(probes, ref, escape_radius);
        auto prev_avg_skipped {-1.0};
        CT prev_exp;
        auto lower_exp {params.lower_exp};
        auto upper_exp {params.upper_exp};
        constexpr auto UPPER_LIMIT {32uz};
        for (auto iter : std::views::iota(0uz, UPPER_LIMIT)) {
            auto middle {(upper_exp + lower_exp) / 2.0};

            auto epsilon = static_cast<ComplexValueTypeT<T>>(std::pow(10.0, middle));
            compute_manual(epsilon, ref, max_dc);
            if (upper_exp - lower_exp < params.convergence_radius) {
                logging::trace( "BLA search iter {}: epsilon=10^{} (converged)", iter, middle);
                break;
            }

            auto total_skipped {compute_skipped_iterations(probes, ref, escape_radius, params.tolerance)};
            if (!total_skipped) {
                logging::trace( "BLA search iter {}: epsilon=10^{} too high", iter, middle);
                upper_exp = middle;
                continue;
            }

            auto avg_skipped = *total_skipped / static_cast<double>(probes.size());
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
    auto get_approximator() const -> Approximator<View, T> {
        return {_first_level, _last_level, get_columns(), get_approximations()};
    }
    auto get_approximator() -> Approximator<View, T> {
        return {_first_level, _last_level, get_columns(), get_approximations()};
    }
    auto resize_approximations(unsigned size) -> void {
        static_cast<Derived*>(this)->resize_approximations(size);
    }
    auto resize_columns(unsigned size) -> void {
        static_cast<Derived*>(this)->resize_columns(size);
    }
    protected:
    auto compute_initial_approximations(CT epsilon, View<const T> ref, T max_dc) -> void {
        static_cast<Derived*>(this)->compute_initial_approximations(epsilon, ref, max_dc);
    }
    auto merge_approximations(std::size_t current_level, std::size_t level_size, T max_dc) -> void {
        static_cast<Derived*>(this)->merge_approximations(current_level, level_size, max_dc);
    }
    auto compute_probe_escape_time(View<const T> probes, View<const T> ref, double escape_radius) -> void {
        static_cast<Derived*>(this)->compute_probe_escape_time(probes, ref, escape_radius);
    }
    auto compute_skipped_iterations(View<const T> probes, View<const T> ref, double escape_radius, double tolerance) -> std::optional<unsigned> {
        return static_cast<Derived*>(this)->compute_skipped_iterations(probes, ref, escape_radius, tolerance);
    }
    auto get_columns() -> View<ColumnInfo> {
        return static_cast<Derived*>(this)->get_columns();
    }
    auto get_columns() const -> View<const ColumnInfo> {
        return static_cast<const Derived*>(this)->get_columns();
    }
    auto get_approximations() -> View<Bla<T>> {
        return static_cast<Derived*>(this)->get_approximations();
    }
    auto get_approximations() const -> View<const Bla<T>> {
        return static_cast<const Derived*>(this)->get_approximations();
    }

    std::size_t _max_ref_size;
    std::size_t _first_level;
    std::size_t _last_level;
};

WF_HD
template <template<typename>typename View, ComplexConcept T>
auto escape_approximate(const T& dc, View<const T> ref, unsigned max_n, double escape_radius,
                        const Approximator<View, T>& approximator)
                        -> std::tuple<Complex<float>, unsigned, unsigned> {
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
class HostCalculator;
template <typename T>
class HostCalculator : public GenericCalculator<HostCalculator<T>, std::span, T> {
    public: 
    using Base = GenericCalculator<HostCalculator<T>, std::span, T>;
    using CT = Base::CT;
    template <typename U>
    using View = std::span<U>;

    protected:
    std::vector<ColumnInfo>     _columns;
    std::vector<Bla<T>>         _working_approximations;
    std::vector<Bla<T>>         _approximations;
    std::vector<unsigned>       _true_escape_times;

    public:
    HostCalculator( std::size_t first_level)
        : Base(first_level)
    {};
    auto resize_columns(unsigned size) -> void {
        _columns.resize(size);
        _working_approximations.resize(size); // Needs to be the same size
    }
    auto resize_approximations(unsigned size) -> void {
        _approximations.resize(size);
    }
    auto compute_initial_approximations(CT epsilon, View<const T> ref, T max_dc) -> void {
        for (auto m : std::views::iota(1uz, ref.size() - 1)) {
            Bla<T> bla {epsilon, ref, max_dc, static_cast<unsigned>(m), static_cast<unsigned>(m + 1)};
            _working_approximations.at(m - 1) = bla;
            if (0 == this->_first_level) {
                auto* ptr {this->get_approximator().approximation_at(m, 0)};
                if (ptr) { *ptr = bla; }
            }
        }
    }
    auto merge_approximations(std::size_t current_level, std::size_t level_size, T max_dc) -> void {
        for (auto k : std::views::iota(0uz, level_size) | std::views::stride(2)) {
            auto bla {Bla<T>::merge(max_dc, _working_approximations.at(k), _working_approximations.at(k+1))};
            _working_approximations.at(k/2) = bla;
            if (current_level >= this->_first_level) {
                auto m {1 + (k / 2) * (1uz << current_level)};
                auto* ptr {this->get_approximator().approximation_at(m, current_level)};
                if (ptr) { *ptr = bla; }
            }
        }
    }
    auto compute_probe_escape_time(View<const T> probes, View<const T> ref, double escape_radius) -> void {
        _true_escape_times.clear();
        _true_escape_times.reserve(probes.size());
        std::ranges::transform(probes, std::back_inserter(_true_escape_times),
            [&ref, &escape_radius](T p) -> unsigned { return escape_perturbed<T>(p, ref, static_cast<unsigned>(ref.size()), escape_radius).second; });
    }
    auto compute_skipped_iterations(View<const T> probes, View<const T> ref, double escape_radius, double tolerance) -> std::optional<unsigned> {
        auto total_skipped {0u};
        for (auto&& [i, probe] : probes | std::views::enumerate) {
            auto [_, approx_escape_time, skipped] = escape_approximate(probe, View<const T>(ref), static_cast<unsigned>(ref.size()), escape_radius, this->get_approximator());

            using std::abs;
            if (abs(static_cast<double>(approx_escape_time) / static_cast<double>(_true_escape_times[i]) - 1.0) > tolerance) {
                return std::nullopt;
            }
            total_skipped += skipped;
        }
        return total_skipped;
    }
    auto get_columns() -> View<ColumnInfo> {
        return _columns;
    }
    auto get_columns() const -> View<const ColumnInfo> {
        return _columns;
    }
    auto get_approximations() -> View<Bla<T>> {
        return _approximations;
    }
    auto get_approximations() const -> View<const Bla<T>> {
        return _approximations;
    }
};

template<typename T>
using Calculator = HostCalculator<T>;

} // namespace wacfrac::bla
