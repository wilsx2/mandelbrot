#pragma once

#include "wacfrac/complex_concept.hpp"

#include <cmath>
#include <cstddef>
#include <optional>
#include <ranges>
#include <span>
#include <sycl/sycl.hpp>
#include <wacfrac/buffer.hpp>
#include <wacfrac/log.hpp>
#include <wacfrac/orbit.hpp>
#include <wacfrac/types.hpp>

namespace wacfrac::bla
{

struct ColumnInfo {
    std::size_t start;
    std::size_t count;

    SYCL_EXTERNAL
    static auto compute_start(unsigned m, std::size_t first_level, std::size_t last_level) -> std::size_t
    {
        auto start{0ull};
        if (m > 1u && first_level <= last_level) {
            auto n{m - 2};
            start = last_level - first_level + 1;
            for (auto l{first_level}; l <= last_level; ++l) {
                start += (n >> l);
            }
        }
        return start;
    }
    SYCL_EXTERNAL
    static auto compute_count(std::size_t m, std::size_t first_level, std::size_t last_level) -> std::size_t
    {
        auto val{m - 1};
        unsigned cz{0};
        if (val == 0) {
            cz = 64;
        } else {
            while ((val & 1) == 0) {
                val >>= 1;
                ++cz;
            }
        }
        auto count{(cz >= first_level && first_level <= last_level)
                       ? 1 + std::min(cz - first_level, last_level - first_level)
                       : 0ull};
        return count;
    }
};

template <ComplexConcept T> struct Bla {
    using CT = ComplexValueTypeT<T>;
    T a, b;
    CT r;
    Bla() = default;
    SYCL_EXTERNAL
    Bla(T a, T b, CT r)
        : a(a),
          b(b),
          r(r)
    {
    }
    SYCL_EXTERNAL
    Bla(CT epsilon, std::span<const T> ref, T max_dc, unsigned m, unsigned n)
    {
        using std::abs;
        auto l{n - m};
        a = T{2.0, 0.0} * ref[m] * static_cast<CT>(l);
        b = T{static_cast<CT>(l), 0.0};
        auto denom = abs(a);
        r = denom > CT{} ? (epsilon * abs(ref[n]) - abs(b) * abs(max_dc)) / denom : -abs(max_dc);
    }
    SYCL_EXTERNAL
    auto is_valid(T dzm) const -> bool
    {
        using std::norm;
        return r > CT{} && norm(dzm) < r * r;
    }
    SYCL_EXTERNAL
    auto approximate_dzn(T dzm, T dc) const -> T { return a * dzm + b * dc; }
    SYCL_EXTERNAL
    static auto merge(const T& max_dc, const Bla& x, const Bla& y) -> Bla
    {
        using std::abs;
        auto a{x.a * y.a};
        auto b{y.a * x.b + y.b};
        auto denom = abs(a);
        auto r = denom > CT{} ? std::min(x.r, (y.r - abs(b) * abs(max_dc)) / denom)
                              : std::min(x.r, y.r - abs(b) * abs(max_dc));
        return {a, b, r};
    }
};

template <ComplexConcept T> struct Approximator {
    using CT = ComplexValueTypeT<T>;
    std::size_t first_level;
    std::size_t last_level;
    std::span<const ColumnInfo> columns;
    std::span<Bla<T>> approximations;

    SYCL_EXTERNAL
    auto approximation_exists(unsigned m, std::size_t level) const -> bool
    {
        return level >= first_level && m > 0 && m - 1 < columns.size() && level - first_level < columns[m - 1].count;
    }
    SYCL_EXTERNAL
    auto approximation_at(unsigned m, std::size_t level) const -> Bla<T>*
    {
        if (approximation_exists(m, level))
            return &approximations[columns[m - 1].start + level - first_level];
        return nullptr;
    }
    SYCL_EXTERNAL
    auto approximate_dzn(T dzm, unsigned m, T dc) const -> std::optional<std::pair<T, unsigned>>
    {
        auto bla{approximation_at(m, first_level)};
        if (!bla || !bla->is_valid(dzm))
            return std::nullopt;

        auto n = m + 1;
        for (auto level = static_cast<std::ptrdiff_t>(last_level); level >= static_cast<std::ptrdiff_t>(first_level);
             --level) {
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

struct Config {
    std::size_t first_level{2};
    double lower_exp{-1028};
    double upper_exp{-1};
    double tolerance{1e-8};
    double convergence_radius = 1e-6;
};

template <ComplexConcept T> class Calculator
{
public:
    using CT = ComplexValueTypeT<T>;
    Calculator(sycl::queue& queue, DeviceArena& arena, const Config& params)
        : _queue{queue},
          _params(params),
          _arena{arena},
          _last_level{0},
          _total_skipped{nullptr},
          _tolerance_failed{nullptr}
    {
    }
    auto allocate_buffers(std::size_t ref_size) -> void
    {
        _last_level =
            ref_size < 3 ? std::size_t{0} : static_cast<std::size_t>(std::log2(static_cast<double>(ref_size)));
        auto max_n{ref_size - 1};
        _columns = _arena.allocate<ColumnInfo>(max_n);
        _current_working = _arena.allocate<Bla<T>>(max_n - 1);
        _next_working = _arena.allocate<Bla<T>>(max_n - 1);
        initialize_columns(max_n);
        auto last_i{ColumnInfo::compute_start(max_n, _params.first_level, _last_level)};
        _approximations = _arena.allocate<Bla<T>>(last_i);
    }
    auto compute_manual(CT epsilon, std::span<const T> ref, T max_dc) -> void
    {
        if (ref.size() < 3)
            return;
        if (_last_level == 0) {
            allocate_buffers(ref.size());
        }

        auto level_size{ref.size() - 2};
        compute_initial_approximations(epsilon, ref, max_dc);
        for (auto i{1ull}; level_size >= 2; ++i) {
            auto even_size{level_size & ~1ull};
            merge_approximations(i, even_size, max_dc);
            std::swap(_current_working, _next_working);
            level_size /= 2;
        }
    }
    auto compute_search(std::span<const T> probes, T max_dc, std::span<const T> ref, double escape_radius = 2.0) -> void
    {
        logging::info("Searching for optimal BLA epsilon: tolerance={} probes={} range=10^[{}, {}]", _params.tolerance,
                      probes.size(), _params.lower_exp, _params.upper_exp);

        compute_probe_escape_time(probes, ref, escape_radius);
        logging::trace("BLA search true escape times:");
        for (std::size_t i{0}; i < probes.size(); ++i) {
            logging::trace("  probe[{}]: ({}, {}) = {}", i, static_cast<double>(probes[i].real()),
                           static_cast<double>(probes[i].imag()), _true_escape_times[i]);
        }
        logging::trace("BLA search max_dc = ({}, {})", static_cast<double>(max_dc.real()),
                       static_cast<double>(max_dc.imag()));
        auto prev_avg_skipped{-1.0};
        CT prev_exp;
        auto lower_exp{_params.lower_exp};
        auto upper_exp{_params.upper_exp};
        constexpr auto UPPER_LIMIT{32ull};
        for (auto iter : std::views::iota(0ull, UPPER_LIMIT)) {
            auto middle{(upper_exp + lower_exp) / 2.0};

            auto epsilon = static_cast<ComplexValueTypeT<T>>(std::pow(10.0, middle));
            compute_manual(epsilon, ref, max_dc);
            if (upper_exp - lower_exp < _params.convergence_radius) {
                logging::trace("BLA search iter {}: epsilon=10^{} (converged)", iter, middle);
                break;
            }

            compute_skipped_iterations(probes, ref, escape_radius, _params.tolerance);
            if (*_tolerance_failed) {
                logging::trace("BLA search iter {}: epsilon=10^{} too high", iter, middle);
                upper_exp = middle;
                continue;
            }

            auto avg_skipped = *_total_skipped / static_cast<double>(probes.size());
            if (avg_skipped >= prev_avg_skipped) {
                logging::trace("BLA search iter {}: epsilon=10^{} avg_skipped={} (improving)", iter, middle,
                               avg_skipped);
                prev_exp = epsilon;
                lower_exp = middle;
            } else {
                logging::trace("BLA search iter {}: epsilon=10^{} avg_skipped={} (found max)", iter, middle,
                               avg_skipped);
                compute_manual(prev_exp, ref, max_dc);
                break;
            }
        }
        logging::info("BLA epsilon search complete");
    }
    auto get_approximator() const -> Approximator<T>
    {
        return {_params.first_level, _last_level, _columns, _approximations};
    }
    auto get_approximator() -> Approximator<T> { return {_params.first_level, _last_level, _columns, _approximations}; }

private:
    auto initialize_columns(std::size_t max_n) -> void
    {
        auto columns{_columns};
        auto first_level{_params.first_level};
        auto last_level{_last_level};

        _queue
            .parallel_for(max_n,
                          [=](sycl::id<1> id) {
                              auto m{id + 1};
                              columns[id] = {ColumnInfo::compute_start(m, first_level, last_level),
                                             ColumnInfo::compute_count(m, first_level, last_level)};
                          })
            .wait();
    }
    auto compute_initial_approximations(CT epsilon, std::span<const T> ref, T max_dc) -> void
    {
        auto first_level{_params.first_level};
        auto approximator = get_approximator();
        auto working = _current_working;

        _queue
            .parallel_for(ref.size() - 2,
                          [=](sycl::id<1> id) {
                              auto m{id + 1};
                              Bla<T> bla{epsilon, ref, max_dc, static_cast<unsigned>(m), static_cast<unsigned>(m + 1)};
                              working[m - 1] = bla;
                              if (0 == first_level) {
                                  auto* ptr{approximator.approximation_at(m, 0)};
                                  if (ptr) {
                                      *ptr = bla;
                                  }
                              }
                          })
            .wait();
    }
    auto merge_approximations(std::size_t current_level, std::size_t level_size, T max_dc) -> void
    {
        auto approximator = get_approximator();
        auto working = _current_working;
        auto next_working = _next_working;
        auto first_level = _params.first_level;

        _queue
            .parallel_for(level_size / 2,
                          [=](sycl::id<1> id) {
                              auto k{id * 2};
                              auto bla{Bla<T>::merge(max_dc, working[k], working[k + 1])};
                              next_working[k / 2] = bla;

                              if (current_level >= first_level) [[likely]] {
                                  auto m{1 + (k / 2) * (1ull << current_level)};
                                  auto* ptr{approximator.approximation_at(m, current_level)};
                                  if (ptr) {
                                      *ptr = bla;
                                  }
                              }
                          })
            .wait();
    }
    auto compute_probe_escape_time(std::span<const T> probes, std::span<const T> ref, double escape_radius) -> void
    {
        if (probes.size() > _true_escape_times.size()) {
            _true_escape_times = _arena.allocate<unsigned>(probes.size());
        }

        auto escape_times = _true_escape_times;

        _queue
            .parallel_for(
                probes.size(),
                [=](sycl::id<1> id) {
                    escape_times[id] =
                        escape_perturbed<T>(probes[id], ref, static_cast<unsigned>(ref.size()), escape_radius).second;
                })
            .wait();
    }
    auto compute_skipped_iterations(std::span<const T> probes, std::span<const T> ref, double escape_radius,
                                    double tolerance) -> void
    {
        if (_total_skipped == nullptr)
            _total_skipped = _arena.allocate<unsigned>();
        if (_tolerance_failed == nullptr)
            _tolerance_failed = _arena.allocate<unsigned>();

        *_total_skipped = 0u;
        *_tolerance_failed = 0u;

        auto approximator{get_approximator()};
        auto escape_times{_true_escape_times};
        auto skipped{_total_skipped};
        auto tolerance_failed{_tolerance_failed};

        _queue
            .parallel_for(
                probes.size(),
                [=](sycl::id<1> id) {
                    sycl::atomic_ref<unsigned, sycl::memory_order_relaxed, sycl::memory_scope_work_group> skipped_atom{
                        *skipped};
                    sycl::atomic_ref<unsigned, sycl::memory_order_relaxed, sycl::memory_scope_work_group>
                        tolerance_atom{*tolerance_failed};

                    auto [_, approx_escape_time, skipped] =
                        escape_approximate(probes[id], std::span<const T>(ref), static_cast<unsigned>(ref.size()),
                                           escape_radius, approximator);

                    using std::abs;
                    if (abs(static_cast<double>(approx_escape_time) / static_cast<double>(escape_times[id]) - 1.0) >
                        tolerance) {
                        tolerance_atom.store(1);
                        return;
                    }
                    skipped_atom.fetch_add(skipped);
                })
            .wait();
    }

    sycl::queue& _queue;
    DeviceArena& _arena;
    Config _params;
    std::size_t _last_level;
    std::span<ColumnInfo> _columns;
    std::span<Bla<T>> _current_working;
    std::span<Bla<T>> _next_working;
    std::span<Bla<T>> _approximations;
    std::span<unsigned> _true_escape_times;
    unsigned* _total_skipped;
    unsigned* _tolerance_failed; // NOTE: Used as a bool. Bool atomic_refs are invalid with SYCL
};

template <ComplexConcept T, typename Ref, typename Approx>
SYCL_EXTERNAL auto escape_approximate(const T& dc, Ref ref, unsigned max_n, double escape_radius,
                                      const Approx& approximator) -> std::tuple<Complex<float>, unsigned, unsigned>
{
    unsigned ref_n{0u};
    unsigned skipped{0u};
    T dz{0.0};
    auto [z, n] = escape_generic(T{}, max_n, escape_radius, [&](T& z, unsigned& n) {
        auto approximation{approximator.approximate_dzn(dz, ref_n, dc)};
        if (approximation) {
            auto m{ref_n};
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

} // namespace wacfrac::bla
