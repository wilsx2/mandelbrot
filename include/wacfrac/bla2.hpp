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

// logging::debug( "Computing BLA with epsilon={} first_level={} ref.size={}", epsilon, first_level, ref.size());
// logging::debug( "BLA computed: {} coefficients across {} columns, levels {}-{}", _approximations.size(), _columns.size(), _first_level, _last_level);

namespace wacfrac {

template <ComplexConcept T = SingleComplex>
class BivariateLinearApproximator {
    public:
    using CT = ComplexValueTypeT<T>;
    struct Foo {
        CT epsilon;
        std::span<const T> ref;
        T max_dc;
    };
    BivariateLinearApproximator(std::size_t first_level = 0); 
    BivariateLinearApproximator(std::size_t first_level, std::size_t max_n); 
    auto compute_manual(CT epsilon, std::span<const T> ref, T max_dc) -> void;
    auto compute_local(std::span<const T> ref, T max_dc) -> void;
    // auto compute_search(...) -> void;
    auto resize(std::size_t max_n) -> void;

    auto approximate_dzn(T dzm, unsigned m, T dc) const -> std::optional<std::pair<T, unsigned>>;

    private:
    struct Approximation {
        T a, b;
        CT r;
        Approximation() = default;
        Approximation(const Foo& foo, unsigned m, unsigned n);
        auto is_valid(T dzm) const -> bool;
        auto approximate_dzn(T dzm, T dc) const -> T;
        auto merge(const T& max_dc, const Approximation& x, const Approximation& y) const -> Approximation;
    };

    struct ColumnInfo {
        std::size_t first;
        std::size_t count;
    };
    auto approximation_exists(unsigned m, std::size_t level) const -> bool;
    auto approximation_at(unsigned m, std::size_t level) const -> const Approximation*; // maybe delete?
    auto approximation_at(unsigned m, std::size_t level) -> Approximation*;

    std::size_t _first_level;
    std::size_t _last_level;
    std::vector<Approximation> _approximations;
    std::vector<ColumnInfo> _columns;
};

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::resize(std::size_t max_n) -> void {
    // TODO: Make work if max_n is greater than reference size; reduce allocs
    auto i {0uz};
    for (auto m : std::views::iota(1uz, max_n)) {
        auto cz {static_cast<unsigned>(std::countr_zero(m - 1))};
        auto size {cz >= _first_level
            ? 1 + std::min(cz - _first_level, _last_level - _first_level)
            : 0uz};
        _columns.at(m - 1) = {i, size};
        i += size;
    }
    _approximations.resize(i);
}

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::compute_manual(CT epsilon, std::span<const T> ref, T max_dc) -> void {
    // TODO: Only resize if we don't have space
    resize(ref.size() - 1);
    std::vector<Approximation> current_level (ref.size() - 2);
    for (auto m : std::views::iota(1uz, ref.size() - 1)) {
        auto bla = compute_bla(epsilon, max_dc, m, m + 1);
        current_level.at(m - 1) = bla;
        if (0 == _first_level) {
            *approximation_at(m, 0) = bla;
        }
    }

    for (auto i {1uz}; current_level.size() >= 2; ++i) {
        auto even_size {current_level.size() & ~1uz};
        for (auto k : std::views::iota(0uz, even_size) | std::views::stride(2)) {
            auto bla = Approximation::merge(max_dc, current_level.at(k), current_level.at(k+1));
            current_level.at(k/2) = bla;
            if (i >= _first_level) {
                auto m {1 + (k / 2) * (1uz << i)};
                *approximation_at(m, i) = bla;
            }
        }
        current_level.resize(current_level.size()/2);
    }
}

template <ComplexConcept T>
auto BivariateLinearApproximator<T>::compute_local(std::span<const T> ref, T max_dc) -> void {
    auto local_epsilon {T{}}; // TODO: Implement
    compute_manual(ref, max_dc);
}

/*
template <ComplexConcept T>
auto BivariateLinearApproximator<T>::compute_search(
    double lower_exp, double upper_exp, double tolerance,
    const std::vector<T>& probes, T max_dc, const std::vector<T>& ref, std::size_t first_level, double escape_radius) -> void
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

        auto epsilon = static_cast<ComplexValueTypeT<T>>(std::pow(10.0, middle));
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
*/

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

    // THIS SHOULD NOT BE POSSIBLE, EXISTS IN EARLIER IMPLEMENTATION
    // if (n >= _ref->get().size())
    //     return std::nullopt;

    return {{bla->approximate_dzn(dzm, dc), n}};
}

template <ComplexConcept T>
BivariateLinearApproximator<T>::Approximation::Approximation(const Foo& foo, unsigned m, unsigned n) {
    using std::abs;
    auto l {n - m};
    a = T{2.0, 0.0} * foo.ref.at(m) * static_cast<CT>(l);
    b = T{static_cast<CT>(l), 0.0};
    auto denom = abs(a);
    r = denom > CT{}
        ? (foo.epsilon * abs(foo.ref.at(n)) - abs(b) * abs(foo.max_dc)) / denom
        : -abs(foo.max_dc);
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
auto BivariateLinearApproximator<T>::Approximation::merge(const T& max_dc, const Approximation& x, const Approximation& y) const -> Approximation {
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

} // namespace wacfrac
