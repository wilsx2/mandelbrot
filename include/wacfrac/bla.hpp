#pragma once

#include <wacfrac/orbit.hpp>
#include <complex>
#include <vector>
#include <optional>

namespace wacfrac {

template <Complex T = std::complex<long double>>
class BivariateLinearApproximator {
    public:
    BivariateLinearApproximator(double epsilon, T max_dc, const std::vector<T>& ref, std::size_t first_level = 0);
    BivariateLinearApproximator(double tolerance, const std::vector<T>& probes, T max_dc, const std::vector<T>& ref, std::size_t first_level = 0);
    auto escape_approximate(T dc) const -> std::tuple<T, std::size_t, std::size_t>;

    private:
    BivariateLinearApproximator(const std::vector<T>& ref, std::size_t first_level);

    struct Bla {
        T a, b;
        ComplexValueTypeT<T> r;
        auto is_valid(T dzm) const -> bool {
            using Real = ComplexValueTypeT<T>;
            using std::norm;
            return r > Real{} && norm(dzm) < r*r;
        }
        auto approximate_dzn(T dzm, T dc) const -> T {
            return a*dzm + b*dc;
        }
    };
    auto compute_bla(ComplexValueTypeT<T> epsilon, T max_dc, std::size_t m, std::size_t n) const -> Bla;
    auto compute_blas(ComplexValueTypeT<T> epsilon, T max_dc) -> void;
    auto merge_blas(T max_dc, const Bla& x, const Bla& y) const -> Bla;
    auto compute_zn(T dc, T dzm, std::size_t m) const -> std::optional<std::pair<T, std::size_t>>;

    struct ColumnInfo {
        std::size_t first;
        std::size_t count;
    };
    auto bla_exists(std::size_t m, std::size_t level) const -> bool;
    auto bla_at(std::size_t m, std::size_t level) const -> const Bla*;
    auto bla_at(std::size_t m, std::size_t level) -> Bla*;

    const std::vector<T>& _ref;
    std::size_t _first_level;
    std::size_t _last_level;
    std::vector<Bla> _blas;
    std::vector<ColumnInfo> _columns;
};

} // namespace wacfrac
