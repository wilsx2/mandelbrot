#pragma once

#include <wacfrac/orbit.hpp>
#include <vector>
#include <optional>

namespace wacfrac {

template <Complex T = SingleComplex>
class BivariateLinearApproximator {
    public:
    BivariateLinearApproximator(ComplexValueTypeT<T> epsilon, T max_dc, const std::vector<T>& ref, std::size_t first_level = 0, double escape_radius = 2.0);
    BivariateLinearApproximator(double lower_exp, double upper_exp, double tolerance,
                                const std::vector<T>& probes, T max_dc, const std::vector<T>& ref, std::size_t first_level = 0, double escape_radius = 2.0);
    auto escape_approximate(T dc) const -> std::tuple<T, unsigned, unsigned>;

    private:
    BivariateLinearApproximator() = default;
    BivariateLinearApproximator(const std::vector<T>& ref, std::size_t first_level, double escape_radius = 2.0);

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
    auto compute_bla(ComplexValueTypeT<T> epsilon, T max_dc, unsigned m, unsigned n) const -> Bla;
    auto compute_blas(ComplexValueTypeT<T> epsilon, T max_dc) -> void;
    auto merge_blas(T max_dc, const Bla& x, const Bla& y) const -> Bla;
    auto compute_zn(T dc, T dzm, unsigned m) const -> std::optional<std::pair<T, unsigned>>;

    struct ColumnInfo {
        std::size_t first;
        std::size_t count;
    };
    auto bla_exists(unsigned m, std::size_t level) const -> bool;
    auto bla_at(unsigned m, std::size_t level) const -> const Bla*;
    auto bla_at(unsigned m, std::size_t level) -> Bla*;

    std::optional<std::reference_wrapper<const std::vector<T>>> _ref;
    std::size_t _first_level;
    std::size_t _last_level;
    std::vector<Bla> _blas;
    std::vector<ColumnInfo> _columns;
    double _escape_radius;
};

} // namespace wacfrac
