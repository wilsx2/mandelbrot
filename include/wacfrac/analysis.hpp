#pragma once

#include "wacfrac/types.hpp"
#include <cstddef>
#include <vector>

namespace wacfrac {

auto find_nucleus(MultiComplex c, std::size_t period, std::size_t max_iterations) -> MultiComplex;

auto find_period_ball(MultiComplex c0, MultiFloat dx, MultiFloat dy, std::size_t max_iterations, bool do_cont) -> std::vector<std::size_t>;

template<Complex T, typename F = double>
auto is_reference_degenerate(const std::vector<T>& ref, F tolerance = 1e-3) -> bool {
    return std::ranges::all_of(ref, [tolerance](auto z) {
        using std::abs;
        using boost::multiprecision::abs;
        return abs(z) < tolerance;
    });
}

} // namespace wacfrac
