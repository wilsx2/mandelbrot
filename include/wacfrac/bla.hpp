#pragma once

#include <wacfrac/orbit.hpp>
#include <complex>
#include <vector>
#include <optional>

namespace wacfrac {

template <Complex T = std::complex<long double>>
class bivariate_linear_approximator {
    public:
    bivariate_linear_approximator(double epsilon, T max_dc, const std::vector<T>& ref, std::size_t first_level = 0);
    bivariate_linear_approximator(double initial_epsilon, double tolerance, const std::vector<T>& probes, T max_dc, const std::vector<T>& ref, std::size_t first_level = 0);
    auto escape_approximate(T dc) const -> std::tuple<T, std::size_t, std::size_t>;

    private:
    bivariate_linear_approximator(const std::vector<T>& ref, std::size_t first_level);

    struct bla {
        T a, b;
        complex_value_type_t<T> r;
        auto is_valid(T dzm) const -> bool {
            using Real = complex_value_type_t<T>;
            using std::norm;
            return r > Real{} && norm(dzm) < r*r;
        }
        auto approximate_dzn(T dzm, T dc) const -> T {
            return a*dzm + b*dc;
        }
    };
    auto compute_bla(double epsilon, T max_dc, std::size_t m, std::size_t n) const -> bla;
    auto compute_blas(double epsilon, T max_dc) -> void;
    auto merge_blas(T max_dc, const bla& x, const bla& y) const -> bla;
    auto compute_zn(T dc, T dzm, std::size_t m) const -> std::optional<std::pair<T, std::size_t>>;

    struct column_info {
        std::size_t first;
        std::size_t count;
    };
    auto bla_exists(std::size_t m, std::size_t level) const -> bool;
    auto bla_at(std::size_t m, std::size_t level) const -> const bla*;
    auto bla_at(std::size_t m, std::size_t level) -> bla*;

    const std::vector<T>& _ref;
    std::size_t _first_level;
    std::size_t _last_level;
    std::vector<bla> _blas;
    std::vector<column_info> _columns;
};

} // namespace wacfrac
