#include "wacfrac/analysis.hpp"

#include "wacfrac/log.hpp"
#include "wacfrac/orbit.hpp"
#include "wacfrac/types.hpp"

#include <algorithm>
#include <cstddef>
#include <ranges>

namespace wacfrac
{

static auto lemniscate_curve(MultiComplex c, std::size_t period) -> std::pair<MultiComplex, MultiComplex>
{
    MultiComplex z{0};
    MultiComplex dz{0};
    for (auto j{0uz}; j < period; ++j) {
        dz = 2 * z * dz + 1;
        z = z * z + c;
    }

    return {z, dz};
}

// https://www.mrob.com/pub/muency/newtonraphsonmethod.html
auto find_nucleus(MultiComplex c, std::size_t period, unsigned max_iterations) -> MultiComplex
{
    auto square_tolerance = MultiFloat{1.0} / pow(MultiFloat{10.0}, 2 * c.precision());
    logging::debug("Finding nucleus at ({}) period={} max_iterations={}", c, period, max_iterations);
    for (unsigned i{0u}; i < max_iterations; ++i) {
        auto [z, dz] = lemniscate_curve(c, period);

        MultiComplex correction = z / dz;
        if (norm(correction) <= square_tolerance) {
            logging::debug("Nucleus converged in {} iterations at ({})", i + 1, c);
            break;
        }

        c -= correction;
    }

    return c;
}

PeriodFinder::PeriodFinder(MultiComplex c0, MultiFloat dx, MultiFloat dy, std::size_t max_period)
    : r0(std::min(abs(dx), abs(dy))),
      r(r0),
      c0(c0),
      z(0),
      dz(0),
      k(1),
      n(max_period)
{
}

auto PeriodFinder::next() -> std::size_t
{
    logging::trace("Searching for period in the range [{},{})", k, n);
    while (k++ < n) {
        az = abs(z);
        adz = abs(dz);
        r = r * r + 2 * (az + r0 * adz) * r + r0 * r0 * adz * adz;
        dz = 2 * z * dz + MultiComplex{1};
        z = z * z + c0;

        if (r + r0 * adz > az) {
            logging::trace("Found period {}", k);
            return k;
        }
        if (az > max_r || r > max_r)
            break;
    }
    logging::debug("No periods found");
    return 0uz;
}

} // namespace wacfrac
