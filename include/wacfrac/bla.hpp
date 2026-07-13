#pragma once

#include "wacfrac/macros.hpp"
#include "wacfrac/buffer.hpp"
#include "wacfrac/executor.hpp"
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
    template<typename ViewType>
    WF_HD
    Bla(CT epsilon, ViewType ref, T max_dc, unsigned m, unsigned n) {
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
    auto approximation_at(unsigned m, std::size_t level) const -> Bla<T>* {
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
        for (auto level = static_cast<std::ptrdiff_t>(last_level); level >= static_cast<std::ptrdiff_t>(first_level); --level) {
            auto next_bla = approximation_at(m, static_cast<std::size_t>(level));
            if (next_bla && next_bla->is_valid(dzm)) {
                bla = next_bla;
                n = m + (1u << level);
                break;
            }
        }

        return {{bla->approximate_dzn(dzm, dc), n}};
    }
};

template <template<typename>typename Buffer, ExecutorLike Executor, ComplexConcept T>
requires BufferLike<Buffer>
class GenericCalculator {
    public:
    using CT = ComplexValueTypeT<T>;
    template<typename U>
    using View = typename Buffer<U>::View;
    GenericCalculator(std::size_t first_level)
        : _max_ref_size(0)
        , _first_level(first_level)
        , _last_level(0)
    {}
    auto resize_for_ref(std::size_t ref_size) -> void {
        _max_ref_size = ref_size;
        _last_level = ref_size < 3 ? std::size_t{0} : static_cast<std::size_t>(std::log2(static_cast<double>(ref_size)));
        auto max_n {ref_size - 1};
        _columns.resize(max_n);
        _current_working.resize(max_n - 1);
        _next_working.resize(_current_working.size() / 2);
        // TODO: Move to Executor
        _columns[max_n - 1] = {0, 0};
        auto i {0ull};
        for (auto m : std::views::iota(1ull, max_n)) { // 
            auto cz {static_cast<unsigned>(std::countr_zero(m - 1))};
            auto size {(cz >= _first_level && _first_level <= _last_level)
                ? 1 + std::min(cz - _first_level, _last_level - _first_level)
                : 0ull};
            _columns[m - 1] = {i, size};
            i += size;
        }
        //
        _approximations.resize(i);
    }
    auto compute_manual(CT epsilon, View<const T> ref, T max_dc) -> void {
        if (ref.size() < 3)
            return;
        if (ref.size() > _max_ref_size) {
            resize_for_ref(ref.size());
        }

        auto level_size {ref.size() - 2};
        compute_initial_approximations(epsilon, ref, max_dc);
        for (auto i {1ull}; level_size >= 2; ++i) {
            auto even_size {level_size & ~1ull};
            merge_approximations(i, even_size, max_dc);
            _current_working.swap(_next_working);
            level_size /= 2;
        }
    }
    // TODO: Change to a View of probes so it is truly generic
    auto compute_search(SearchParams params, const std::vector<T>& probes, T max_dc, View<const T> ref, double escape_radius = 2.0) -> void {
        logging::info( "Searching for optimal BLA epsilon: tolerance={} probes={} range=10^[{}, {}]", params.tolerance, probes.size(), params.lower_exp, params.upper_exp);

        compute_probe_escape_time(probes, ref, escape_radius);
        auto prev_avg_skipped {-1.0};
        CT prev_exp;
        auto lower_exp {params.lower_exp};
        auto upper_exp {params.upper_exp};
        constexpr auto UPPER_LIMIT {32ull};
        for (auto iter : std::views::iota(0ull, UPPER_LIMIT)) {
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
    }
    auto get_approximator() const -> Approximator<View, T> {
        return {_first_level, _last_level, _columns.get_view(), _approximations.get_view()};
    }
    auto get_approximator() -> Approximator<View, T> {
        return {_first_level, _last_level, _columns.get_view(), _approximations.get_view()};
    }
    protected:
    auto compute_initial_approximations(CT epsilon, View<const T> ref, T max_dc) -> void {
        _executor(ref.size() - 2,
            [epsilon, ref, max_dc,
             first_level = _first_level,
             approximator = get_approximator(),
             working = _current_working.get_view()]
            WF_HD (auto tid){
                if (tid + 2 >= ref.size())
                    return;
                auto m {tid + 1};
                Bla<T> bla {epsilon, ref, max_dc, static_cast<unsigned>(m), static_cast<unsigned>(m + 1)};
                working[m - 1] = bla;
                if (0 == first_level) {
                    auto* ptr {approximator.approximation_at(m, 0)};
                    if (ptr) { *ptr = bla; }
                }
            });
    }
    auto merge_approximations(std::size_t current_level, std::size_t level_size, T max_dc) -> void {
        _executor(level_size / 2,
            [current_level, max_dc,
             first_level = _first_level,
             approximator = get_approximator(),
             working = _current_working.get_view(),
             next_working = _next_working.get_view()]
            WF_HD (auto tid){
                auto k {tid * 2};
                if (k >= working.size())
                    return;

                auto bla {Bla<T>::merge(max_dc, working[k], working[k+1])};
                next_working[k/2] = bla;
                if (current_level >= first_level) { // NOTE: Can precompute this
                    auto m {1 + (k / 2) * (1ull << current_level)};
                    auto* ptr {approximator.approximation_at(m, current_level)};
                    if (ptr) { *ptr = bla; }
                }
            });
    }
    auto compute_probe_escape_time(View<const T> probes, View<const T> ref, double escape_radius) -> void {
        if (probes.size() > _true_escape_times.size()) {
            _true_escape_times.resize(probes.size());
        }

        _executor(probes.size(),
            [probes, ref, escape_radius,
             escape_times = _true_escape_times.get_view()]
            WF_HD (auto tid){
                if (tid >= probes.size())
                    return;
                escape_times[tid] = escape_perturbed<T>(
                    probes[tid], ref, 
                    static_cast<unsigned>(ref.size()), 
                    escape_radius).second;
            });
    }
    auto compute_skipped_iterations(View<const T> probes, View<const T> ref, double escape_radius, double tolerance) -> std::optional<unsigned> {
        std::atomic total_skipped {0u}; // NOTE: Does not work on GPU. Use storage and WF_STD::atomic. Allocate as member
        std::atomic tolerance_failed {false};
        _executor(probes.size(),
            [probes, ref, escape_radius, tolerance,
             escape_times = _true_escape_times.get_view(),
             tolerance_failed = &tolerance_failed,
             total_skipped = &total_skipped,
             approximator = get_approximator()]
            WF_HD (auto tid){
                if (tid >= probes.size())
                    return;
                auto [_, approx_escape_time, skipped] =
                    escape_approximate(
                        probes[tid], 
                        View<const T>(ref), 
                        static_cast<unsigned>(ref.size()),
                        escape_radius,
                        approximator);

                using std::abs;
                if (abs(static_cast<double>(approx_escape_time) / static_cast<double>(escape_times[tid]) - 1.0) > tolerance) {
                    *tolerance_failed = true; // TODO: Use explicit method calls
                    return;
                }
                *total_skipped += skipped; // TODO: Above
            });

        if (tolerance_failed)
            return std::nullopt;
        return total_skipped;
    }

    std::size_t _max_ref_size;
    std::size_t _first_level;
    std::size_t _last_level;
    Buffer<ColumnInfo> _columns;
    Buffer<Bla<T>>     _current_working;
    Buffer<Bla<T>>     _next_working;
    Buffer<Bla<T>>     _approximations;
    Buffer<unsigned>   _true_escape_times;
    Executor            _executor;
};

template <ComplexConcept T, typename Ref, typename Approx>
WF_HD
auto escape_approximate(const T& dc, Ref ref, unsigned max_n, double escape_radius,
                        const Approx& approximator)
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
using HostCalculator = GenericCalculator<HostBuffer, ParallelExecutor, T>;
template<typename T>
using Calculator = HostCalculator<T>;

} // namespace wacfrac::bla
