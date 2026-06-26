#include "wacfrac/analysis.hpp"
#include "wacfrac/log.hpp"
#include "wacfrac/orbit.hpp"
#include <algorithm>

namespace wacfrac {

static auto lemniscate_curve(MultiComplex c, std::size_t period) -> std::pair<MultiComplex, MultiComplex> {
    MultiComplex z  {0};
    MultiComplex dz {0};
    for (auto j {0uz}; j < period; ++j) {
        dz = 2 * z * dz + 1;
        z  = z * z + c;
    }

    return {z, dz};
}

// https://www.mrob.com/pub/muency/newtonraphsonmethod.html
auto find_nucleus(MultiComplex c, std::size_t period,  std::size_t max_iterations) -> MultiComplex {
    auto square_tolerance = 1.0 / std::pow(10.0, 2.0*c.precision());
    logging::print(logging::Severity::Debug, "Finding nucleus at ({}) period={} max_iterations={}", c, period, max_iterations);
    for (auto i {0uz}; i < max_iterations; ++i) {
        auto [z, dz] = lemniscate_curve(c, period);

        if (square_magnitude(dz) < MultiFloat{1e-30}) {
            logging::print(logging::Severity::Trace, "Nucleus iteration {}: dz too small, perturbing", i);
            c += MultiComplex{1e-3, 1e-3};
            continue;
        }

        MultiComplex correction = z / dz;
        if (square_magnitude(correction) > MultiFloat{16.0}) {
            correction *= MultiFloat{4.0} / sqrt(square_magnitude(correction));
        }
        if (square_magnitude(correction) <= square_tolerance) {
            logging::print(logging::Severity::Debug, "Nucleus converged in {} iterations at ({})", i+1, c);
            break;
        }
        c -= correction;
    }

    return c;
}

// https://fractalforums.org/index.php?topic=3805.msg24312#msg24312
auto find_period_ball(MultiComplex c0, MultiFloat dx, MultiFloat dy, std::size_t max_iterations, bool do_cont) -> std::vector<std::size_t> {
    auto r0 = std::min(abs(dx), abs(dy));

    MultiComplex z{0};
    MultiComplex dz{0};
    MultiFloat r{r0};
    MultiFloat az{0};
    MultiFloat adz{0};

    std::vector<std::size_t> periods;

    constexpr double max_r{1e5};

    for (auto k {1uz}; k <= max_iterations; ++k) {
        r = r * r + 2 * (az + r0 * adz) * r + r0 * r0 * adz * adz;
        dz = 2 * z * dz + MultiComplex{1};
        z = z * z + c0;
        az = abs(z);
        adz = abs(dz);

        if (r + r0 * adz > az) {
            periods.push_back(k);
            logging::print(logging::Severity::Trace, "Found period candidate {} at iteration {}", k, k);
            if (!do_cont) break;
        }

        if (az > max_r || r > max_r) break;
    }

    logging::print(logging::Severity::Debug, "Period ball search found {} candidates", periods.size());
    return periods;
}

} // namespace wacfrac
