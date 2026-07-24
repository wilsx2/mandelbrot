#pragma once

#include "wacfrac/types.hpp"
#include <cstddef>

namespace wacfrac {

auto find_nucleus(MultiComplex c, std::size_t period, unsigned max_iterations) -> MultiComplex;

// https://fractalforums.org/index.php?topic=3805.msg24312#msg24312
struct PeriodFinder {
public: 
    PeriodFinder(MultiComplex c0, MultiFloat dx, MultiFloat dy, std::size_t max_period);
    auto next() -> std::size_t;
private:
    static constexpr double max_r {1e5};
    MultiFloat r0, r, az, adz;
    MultiComplex c0, z, dz;
    std::size_t k, n;
};

} // namespace wacfrac
