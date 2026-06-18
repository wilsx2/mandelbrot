#include "wacfrac/analysis.hpp"
#include "wacfrac/orbit.hpp"

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
    auto square_tolerance = 1.0 / std::pow(10.0, 2.0*c.precision()); // TODO: Worry about precision getting cut
    for (auto i {0uz}; i < max_iterations; ++i) {
        multi_complex z, dz;
        std::tie(z, dz) = lemniscate_curve(c, period);

        multi_complex correction = z / dz;
        if (square_magnitude(correction) <= square_tolerance) {
            break;
        }
        c -= correction;
    }

    return c;
}

// https://math.stackexchange.com/questions/2967515/difference-between-limbs-and-bulbs-in-mandelbrot-set
//auto find_attachment_point(unsigned int p, unsigned int q) {
    // NOTE: p and q must be coprime
    // NOTE: q represents a local period
//}

} // namespace wacfrac
