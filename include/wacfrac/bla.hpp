#pragma once

#include "wacfrac/macros.hpp"
#include "wacfrac/context.hpp"
#include "wacfrac/complex_concept.hpp"
#include <wacfrac/orbit.hpp>
#include <wacfrac/types.hpp>
#include <wacfrac/log.hpp>
#include <optional>
#include <ranges>
#include <cmath>
#include <cstddef>

#if defined(__CUDACC__)
#include <cuda/std/span>
#include <cuda/std/atomic>
#endif

namespace wacfrac::bla {

struct ColumnInfo {
    std::size_t start;
    std::size_t count;

    WF_HD
    static auto compute_start(unsigned m, std::size_t first_level, std::size_t last_level) -> std::size_t {
        auto start {0ull};
        if (m > 1u && first_level <= last_level) {
            auto n {m - 2};
            start = last_level - first_level + 1; 
            for (auto l {first_level}; l <= last_level; ++l) {
                start += (n >> l);
            }
        }
        return start;
    }
    WF_HD
    static auto compute_count(std::size_t m, std::size_t first_level, std::size_t last_level) -> std::size_t {
        auto val {m - 1};
        unsigned cz {0};
        if (val == 0) {
            cz = 64;
        } else {
            while ((val & 1) == 0) { val >>= 1; ++cz; }
        }
        auto count {(cz >= first_level && first_level <= last_level)
            ? 1 + std::min(cz - first_level, last_level - first_level)
            : 0ull};
        return count;
    }
};

template<ComplexConcept T>
struct Bla {
    using CT = ComplexValueTypeT<T>;
    T a, b;
    CT r;
    Bla() = default;
    WF_HD
    Bla(T a, T b, CT r) : a(a), b(b), r(r) {}
    WF_HD
    Bla(CT epsilon, WF_STD::span<const T> ref, T max_dc, unsigned m, unsigned n) {
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

template<ComplexConcept T>
struct Approximator {
    using CT = ComplexValueTypeT<T>;
    std::size_t first_level;
    std::size_t last_level;
    WF_STD::span<const ColumnInfo> columns;
    WF_STD::span<Bla<T>> approximations;

    WF_HD
    auto approximation_exists(unsigned m, std::size_t level) const -> bool {
        return level >= first_level && m > 0 && m - 1 < columns.size() && level - first_level < columns[m - 1].count;
    }
    WF_HD
    auto approximation_at(unsigned m, std::size_t level) const -> Bla<T>* {
        if (approximation_exists(m, level))
            return &approximations[columns[m - 1].start + level - first_level];
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

struct Config {
    std::size_t first_level {2};
    double lower_exp {-1028};
    double upper_exp {-16};
    double tolerance {1e-8};
    double convergence_radius = 1e-3;
};

template <typename Context, ComplexConcept T>
class Calculator {
    public:
    using CT = ComplexValueTypeT<T>;
    template<typename U>
    using Buffer = Context::template Buffer<U>;
    template<typename U>
    using Pointer = Context::template Pointer<U>;
    Calculator(const Config& params, Arena& arena, const Context& ctx = {})
        : _ctx{ctx}
        , _params(params)
        , _arena{arena}
        , _last_level{0}
        , _skipped_atom{nullptr}
        , _tolerance_failed_atom{nullptr}
    { }
    auto allocate_buffers(std::size_t ref_size) -> void {
        _last_level = ref_size < 3 ? std::size_t{0} : static_cast<std::size_t>(std::log2(static_cast<double>(ref_size)));
        auto max_n {ref_size - 1};
        _columns         = _arena.alloc<ColumnInfo>(max_n);
        _current_working = _arena.alloc<Bla<T>>(max_n - 1);
        _next_working    = _arena.alloc<Bla<T>>(max_n - 1);
        initialize_columns(max_n);
        auto last_i {ColumnInfo::compute_start(max_n, _params.first_level, _last_level)};
        _approximations  = _arena.alloc<Bla<T>>(last_i);
    }
    auto compute_manual(CT epsilon, WF_STD::span<const T> ref, T max_dc) -> void {
        if (ref.size() < 3)
            return;
        if (_last_level == 0) {
            allocate_buffers(ref.size());
        }

        auto level_size {ref.size() - 2};
        compute_initial_approximations(epsilon, ref, max_dc);
        for (auto i {1ull}; level_size >= 2; ++i) {
            auto even_size {level_size & ~1ull};
            merge_approximations(i, even_size, max_dc);
            swap(_current_working, _next_working);
            level_size /= 2;
        }
    }
    auto compute_search(WF_STD::span<const T> probes, T max_dc, WF_STD::span<const T> ref, double escape_radius = 2.0) -> void {
        logging::info( "Searching for optimal BLA epsilon: tolerance={} probes={} range=10^[{}, {}]",
                      _params.tolerance, probes.size(), _params.lower_exp, _params.upper_exp);

        compute_probe_escape_time(probes, ref, escape_radius);
        logging::trace("BLA search true escape times:");
        for (std::size_t i {0}; i < probes.size(); ++i) {
            logging::trace("  probe[{}]: ({}, {}) = {}", i,
                static_cast<double>(probes[i].real()),
                static_cast<double>(probes[i].imag()),
                _true_escape_times[i]);
        }
        logging::trace("BLA search max_dc = ({}, {})",
            static_cast<double>(max_dc.real()),
            static_cast<double>(max_dc.imag()));
        auto prev_avg_skipped {-1.0};
        CT prev_exp;
        auto lower_exp {_params.lower_exp};
        auto upper_exp {_params.upper_exp};
        constexpr auto UPPER_LIMIT {32ull};
        for (auto iter : std::views::iota(0ull, UPPER_LIMIT)) {
            auto middle {(upper_exp + lower_exp) / 2.0};

            auto epsilon = static_cast<ComplexValueTypeT<T>>(std::pow(10.0, middle));
            compute_manual(epsilon, ref, max_dc);
            if (upper_exp - lower_exp < _params.convergence_radius) {
                logging::trace( "BLA search iter {}: epsilon=10^{} (converged)", iter, middle);
                break;
            }

            compute_skipped_iterations(probes, ref, escape_radius, _params.tolerance);
            if (_tolerance_failed_atom->load()) {
                logging::trace( "BLA search iter {}: epsilon=10^{} too high", iter, middle);
                upper_exp = middle;
                continue;
            }

            auto avg_skipped = _skipped_atom->load() / static_cast<double>(probes.size());
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
    auto get_approximator() const -> Approximator<T> {
        return {_params.first_level, _last_level, _columns, _approximations};
    }
    auto get_approximator() -> Approximator<T> {
        return {_params.first_level, _last_level, _columns, _approximations};
    }
    public: // TODO: Remove from public API. This is a work around to make NVCC happy with our lambda usage in device code
    auto initialize_columns(std::size_t max_n) -> void {
        auto columns {_columns};
        auto first_level {_params.first_level};
        auto last_level {_last_level};
        _ctx.parallel_for(max_n,
            [max_n,
             first_level,
             last_level,
             columns]
            WF_HD
            (int tid){
                auto m {tid + 1};
                if (m > max_n)
                    return;
                columns[m - 1] = {
                    ColumnInfo::compute_start(m, first_level, last_level),
                    ColumnInfo::compute_count(m, first_level, last_level)
                };
            });
    }
    auto compute_initial_approximations(CT epsilon, WF_STD::span<const T> ref, T max_dc) -> void {
        auto first_level {_params.first_level};
        auto approximator = get_approximator();
        auto working = _current_working;
        _ctx.parallel_for(ref.size() - 2,
            [epsilon,
             ref,
             max_dc,
             first_level,
             approximator,
             working]
            WF_HD
            (int tid){
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
        if (current_level >= _params.first_level) {
            auto approximator = get_approximator();
            auto working = _current_working;
            auto next_working = _next_working;
            _ctx.parallel_for(level_size / 2,
                [current_level,
                 max_dc,
                 approximator,
                 working,
                 next_working]
                WF_HD
                (int tid){
                    auto k {tid * 2};
                    if (k >= working.size())
                        return;

                    auto bla {Bla<T>::merge(max_dc, working[k], working[k+1])};
                    next_working[k/2] = bla;

                    auto m {1 + (k / 2) * (1ull << current_level)};
                    auto* ptr {approximator.approximation_at(m, current_level)};
                    if (ptr) { *ptr = bla; }
                });
        } else {
            auto approximator = get_approximator();
            auto working = _current_working;
            auto next_working = _next_working;
            _ctx.parallel_for(level_size / 2,
                [max_dc,
                 approximator,
                 working,
                 next_working]
                WF_HD
                (int tid){
                    auto k {tid * 2};
                    if (k >= working.size())
                        return;

                    auto bla {Bla<T>::merge(max_dc, working[k], working[k+1])};
                    next_working[k/2] = bla;
                });
        }
    }
    auto compute_probe_escape_time(WF_STD::span<const T> probes, WF_STD::span<const T> ref, double escape_radius) -> void {
        if (probes.size() > _true_escape_times.size()) {
            _true_escape_times = _arena.alloc<unsigned>(probes.size());
        }

        auto escape_times = _true_escape_times;
        _ctx.parallel_for(probes.size(),
            [probes,
             ref,
             escape_radius,
             escape_times]
            WF_HD
            (int tid){
                if (tid >= probes.size())
                    return;
                escape_times[tid] = escape_perturbed<T>(
                    probes[tid], ref, 
                    static_cast<unsigned>(ref.size()), 
                    escape_radius).second;
            });
    }
    auto compute_skipped_iterations(WF_STD::span<const T> probes, WF_STD::span<const T> ref, double escape_radius, double tolerance) -> void {
        if (_skipped_atom == nullptr)
            _skipped_atom = _arena.alloc<WF_STD::atomic<unsigned>>();
        if (_tolerance_failed_atom == nullptr)
            _tolerance_failed_atom = _arena.alloc<WF_STD::atomic<bool>>();

        _skipped_atom->store(0u);
        _tolerance_failed_atom->store(false);

        constexpr std::size_t DIAG_BUF_SIZE {16};
        auto diag_count {_arena.alloc<WF_STD::atomic<unsigned>>()};
        auto diag_buf  {_arena.alloc<int>(DIAG_BUF_SIZE * 4)};
        diag_count->store(0u);

        auto approximator = get_approximator();
        auto escape_times = _true_escape_times;
        auto tolerance_failed = _tolerance_failed_atom;
        auto total_skipped = _skipped_atom;
        _ctx.parallel_for(probes.size(),
            [probes,
             ref,
             escape_radius,
             tolerance,
             escape_times,
             tolerance_failed,
             total_skipped,
             approximator,
             diag_count,
             diag_buf]
            WF_HD
            (int tid){
                if (tid >= probes.size())
                    return;
                auto [_, approx_escape_time, skipped] =
                    escape_approximate(
                        probes[tid], 
                        WF_STD::span<const T>(ref), 
                        static_cast<unsigned>(ref.size()),
                        escape_radius,
                        approximator);

                using std::abs;
                if (abs(static_cast<double>(approx_escape_time) / static_cast<double>(escape_times[tid]) - 1.0) > tolerance) {
                    tolerance_failed->store(true, WF_STD::memory_order_seq_cst); // TODO: Pick a good setting
                    auto idx {diag_count->fetch_add(1u, WF_STD::memory_order_relaxed)};
                    if (idx < DIAG_BUF_SIZE) {
                        diag_buf[idx * 4 + 0] = tid;
                        diag_buf[idx * 4 + 1] = static_cast<int>(escape_times[tid]);
                        diag_buf[idx * 4 + 2] = static_cast<int>(approx_escape_time);
                        diag_buf[idx * 4 + 3] = static_cast<int>(skipped);
                    }
                    return;
                }
                total_skipped->fetch_add(skipped, WF_STD::memory_order_seq_cst);
            });
        auto fail_count {diag_count->load()};
        if (fail_count > 0) {
            for (std::size_t i {0}; i < std::min<std::size_t>(fail_count, DIAG_BUF_SIZE); ++i) {
                auto tid     {diag_buf[i * 4 + 0]};
                auto true_n  {diag_buf[i * 4 + 1]};
                auto approx_n{diag_buf[i * 4 + 2]};
                auto skipped {diag_buf[i * 4 + 3]};
                auto rel_err {true_n > 0
                    ? std::abs(static_cast<double>(approx_n) / static_cast<double>(true_n) - 1.0)
                    : (approx_n == 0 ? 0.0 : 999.0)};
                logging::trace("  Tolerance failed: probe={} true={} approx={} skipped={} rel_error={}",
                               tid, true_n, approx_n, skipped, rel_err);
            }
            if (fail_count > DIAG_BUF_SIZE) {
                logging::trace("  ... and {} more failures", fail_count - DIAG_BUF_SIZE);
            }
        }
    }

    Context _ctx;
    Config _params;
    Arena& _arena;
    std::size_t _last_level;
    WF_STD::span<ColumnInfo> _columns;
    WF_STD::span<Bla<T>>     _current_working;
    WF_STD::span<Bla<T>>     _next_working;
    WF_STD::span<Bla<T>>     _approximations;
    WF_STD::span<unsigned>   _true_escape_times;
    WF_STD::atomic<unsigned>* _skipped_atom;
    WF_STD::atomic<bool>* _tolerance_failed_atom;
};

template <ComplexConcept T, typename Ref, typename Approx>
WF_HD
auto escape_approximate(const T& dc, Ref ref, unsigned max_n, double escape_radius,
                        const Approx& approximator)
                        -> std::tuple<Complex<float>, unsigned, unsigned> {
    unsigned ref_n {0u};
    unsigned skipped {0u};
    T dz {0.0};
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

} // namespace wacfrac::bla
