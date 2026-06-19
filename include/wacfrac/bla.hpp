#pragma once

#include <complex>
#include <vector>
namespace wacfrac {

/*

define dz, dc, and n

bivariate_linear_approximator bla {epsilon, ref}


*/

class bivariate_linear_approximator {
    public:
    static auto find_heuristic_epsilon(const std::vector<std::complex<double>>& probes, double initial_epsilon, double threshold);
    bivariate_linear_approximator(const std::vector<std::complex<double>>& ref, std::complex<double> max_c, std::complex<double> max_dc, double epsilon = 2e-53, std::size_t first_level = 2);
    auto apply(std::complex<double> dc, std::complex<double> dzm, std::size_t m) const -> std::optional<std::pair<std::complex<double>, std::size_t>>;

    private:
    struct bla {
        std::complex<double> a, b;
        double r;
        auto is_valid(std::complex<double> dzm) const -> bool;
        auto approximate_dzn(std::complex<double> dzm, std::complex<double> dc) const -> std::complex<double>;
    };
    auto compute_bla(std::size_t m, std::size_t n) const -> bla; 
    auto merge_blas(const bla& x, const bla& y) const -> bla;

    struct column_info {
        std::size_t first;
        std::size_t count;
    };
    auto bla_exists(std::size_t m, std::size_t level) const -> bool;
    auto bla_at(std::size_t m, std::size_t level) const -> const bla*;
    auto bla_at(std::size_t m, std::size_t level) -> bla*;

    const std::vector<std::complex<double>>& _ref;
    std::complex<double> _max_c;
    std::complex<double> _max_dc;
    double _epsilon;

    std::vector<bla> _blas;
    std::vector<column_info> _columns;
    std::size_t _first_level;
    std::size_t _last_level;
};

} // namespace wacfrac
