#pragma once

#include <complex>
#include <vector>

namespace wacfrac {

class bivariate_linear_approximator {
    public:
    bivariate_linear_approximator(double epsilon, std::complex<long double> max_dc, const std::vector<std::complex<long double>>& ref, std::size_t first_level = 0);
    bivariate_linear_approximator(double initial_epsilon, double tolerance, const std::vector<std::complex<long double>>& probes, std::complex<long double> max_dc, const std::vector<std::complex<long double>>& ref, std::size_t first_level = 0);
    auto escape_approximate(std::complex<long double> dc) const -> std::tuple<std::complex<long double>, std::size_t, std::size_t>;

    private:
    bivariate_linear_approximator(const std::vector<std::complex<long double>>& ref, std::size_t first_level);

    struct bla {
        std::complex<long double> a, b;
        long double r;
        auto is_valid(std::complex<long double> dzm) const -> bool;
        auto approximate_dzn(std::complex<long double> dzm, std::complex<long double> dc) const -> std::complex<long double>;
    };
    auto compute_bla(double epsilon, std::complex<long double> max_dc, std::size_t m, std::size_t n) const -> bla;
    auto compute_blas(double epsilon, std::complex<long double> max_dc) -> void;
    auto merge_blas(std::complex<long double> max_dc, const bla& x, const bla& y) const -> bla;
    auto compute_zn(std::complex<long double> dc, std::complex<long double> dzm, std::size_t m) const -> std::optional<std::pair<std::complex<long double>, std::size_t>>;

    struct column_info {
        std::size_t first;
        std::size_t count;
    };
    auto bla_exists(std::size_t m, std::size_t level) const -> bool;
    auto bla_at(std::size_t m, std::size_t level) const -> const bla*;
    auto bla_at(std::size_t m, std::size_t level) -> bla*;

    const std::vector<std::complex<long double>>& _ref;
    std::size_t _first_level;
    std::size_t _last_level;
    std::vector<bla> _blas;
    std::vector<column_info> _columns;
};

} // namespace wacfrac
