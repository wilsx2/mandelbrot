#include "wacfrac/analysis.hpp"
#include "wacfrac/orbit.hpp"
#include <algorithm>

namespace wacfrac {

static auto lemniscate_curve(multi_complex c, std::size_t period) -> std::pair<multi_complex, multi_complex> {
    multi_complex z  {0};
    multi_complex dz {0};
    for (auto j {0uz}; j < period; ++j) {
        dz = 2 * z * dz + 1;
        z  = z * z + c;
    }

    return {z, dz};
}

// https://www.mrob.com/pub/muency/newtonraphsonmethod.html
auto find_nucleus(multi_complex c, std::size_t period,  std::size_t max_iterations) -> multi_complex {
    auto square_tolerance = 1.0 / std::pow(10.0, 2.0*c.precision());
    for (auto i {0uz}; i < max_iterations; ++i) {
        multi_complex z, dz;
        std::tie(z, dz) = lemniscate_curve(c, period);

        if (square_magnitude(dz) < multi_float{1e-30}) {
            c += multi_complex{1e-3, 1e-3};
            continue;
        }

        multi_complex correction = z / dz;
        if (square_magnitude(correction) > multi_float{16.0}) {
            correction *= multi_float{4.0} / sqrt(square_magnitude(correction));
        }
        if (square_magnitude(correction) <= square_tolerance) {
            break;
        }
        c -= correction;
    }

    return c;
}

// https://fractalforums.org/index.php?topic=3805.msg24312#msg24312
auto find_period_ball(multi_complex c0, multi_float dx, multi_float dy, std::size_t max_iterations, bool do_cont) -> std::vector<std::size_t> {
    auto r0 = std::min(abs(dx), abs(dy));

    multi_complex z{0};
    multi_complex dz{0};
    multi_float r{r0};
    multi_float az{0};
    multi_float adz{0};

    std::vector<std::size_t> periods;

    constexpr double max_r{1e5};

    for (auto k {1uz}; k <= max_iterations; ++k) {
        r = r * r + 2 * (az + r0 * adz) * r + r0 * r0 * adz * adz;
        dz = 2 * z * dz + multi_complex{1};
        z = z * z + c0;
        az = abs(z);
        adz = abs(dz);

        if (r + r0 * adz > az) {
            periods.push_back(k);
            if (!do_cont) break;
        }

        if (az > max_r || r > max_r) break;
    }

    return periods;
}

} // namespace wacfrac
