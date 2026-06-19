#include "wacfrac/bla.hpp"
#include <cstddef>
#include <optional>
#include <ranges>

using namespace wacfrac;

// https://philthompson.me/2023/Faster-Mandelbrot-Set-Rendering-with-BLA-Bivariate-Linear-Approximation.html

bivariate_linear_approximator::bivariate_linear_approximator(const std::vector<std::complex<double>>& ref, std::complex<double> max_c, std::complex<double> max_dc, double epsilon, std::size_t first_level)
    : _ref(ref)
    , _max_c(max_c)
    , _max_dc(max_dc)
    , _epsilon(epsilon)
    , _blas(0)
    , _columns(ref.size() - 2)
    , _first_level(first_level)
    , _last_level(std::log2(_ref.size()))
{
    // Generate column information for quick lookups
    {
        auto i {0uz};
        for (auto m : std::views::iota(1uz, ref.size() - 1)) {
            auto size {1 + std::min(static_cast<std::size_t>(std::countr_zero(m - 1)), _last_level - _first_level)};
            _columns.at(m - 1) = {i, size};
            i += size;
        }
        _blas.resize(i);
    }
    
    // Calculate 1-iteration BLAs
    std::vector<bla> current_level (ref.size() - 2);
    for (auto m : std::views::iota(1uz, ref.size() - 1)) {
        auto bla = compute_bla(m, m + 1);
        current_level.at(m - 1) = bla;
        if (0 == _first_level) {
            *bla_at(m, 0) = bla;
        }
    }
  
    // Merge
    for (auto i {1uz}; current_level.size() >= 2; ++i) {
        auto even_size {current_level.size() & ~1uz};
        for (auto k : std::views::iota(0uz, even_size) | std::views::stride(2)) {
            auto bla = merge_blas(current_level.at(k), current_level.at(k+1));
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

auto bivariate_linear_approximator::apply(std::complex<double> dc, std::complex<double> dzm, std::size_t m) const -> std::optional<std::pair<std::complex<double>, std::size_t>> {
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

auto bivariate_linear_approximator::bla::is_valid(std::complex<double> dzm) const -> bool {
    return r > 0.0 && std::norm(dzm) < r*r;
}
auto bivariate_linear_approximator::bla::approximate_dzn(std::complex<double> dzm, std::complex<double> dc) const -> std::complex<double> {
    return a*dzm + b*dc;
}
auto bivariate_linear_approximator::compute_bla(std::size_t m, std::size_t n) const -> bla {
    auto l {n - m};
    auto a {2.0 * _ref.at(m) * static_cast<double>(l)};
    auto b {static_cast<std::complex<double>>(1.0 * l)};
    auto denom = std::abs(a);
    auto r = denom > 0.0
        ? (_epsilon * std::abs(_ref[n]) - std::abs(b) * std::abs(_max_dc)) / denom
        : -std::abs(_max_dc);
    return {a,b,r};
}
auto bivariate_linear_approximator::merge_blas(const bla& x, const bla& y) const -> bla {
    auto a {x.a * y.a};
    auto b {y.a * x.b + y.b};
    auto denom = std::abs(a);
    auto r = denom > 0.0
        ? std::min(x.r, (y.r - std::abs(b) * std::abs(_max_dc)) / denom)
        : std::min(x.r, y.r - std::abs(b) * std::abs(_max_dc));
    return {a,b,r};
}
auto bivariate_linear_approximator::bla_exists(std::size_t m, std::size_t level) const -> bool {
    return level >= _first_level && m > 0 && m - 1 < _columns.size() && level - _first_level < _columns.at(m - 1).count;
}
auto bivariate_linear_approximator::bla_at(std::size_t m, std::size_t level) const -> const bla* {
    if (bla_exists(m, level))
        return &_blas.at(_columns.at(m - 1).first + level - _first_level);
    return nullptr;
}
auto bivariate_linear_approximator::bla_at(std::size_t m, std::size_t level) -> bla* {
    if (bla_exists(m, level))
        return &_blas.at(_columns.at(m - 1).first + level - _first_level);
    return nullptr;
}
