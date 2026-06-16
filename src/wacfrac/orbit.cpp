#include <wacfrac/orbit.hpp>
#include <utility>

namespace wacfrac
{

// https://fractalforums.org/index.php?topic=4360.0
auto escape_perturbed(const std::vector<std::complex<double>>& ref, std::complex<double> dc, unsigned int max_n, std::complex<double> dz, unsigned int n) -> std::pair<std::complex<double>, unsigned int> {
    auto ref_n {n};
    if (ref_n >= ref.size()) {
        return {{}, 0};
    }

    auto z0 {ref[ref_n] + dz};
    return escape_generic(z0, n, max_n, [&](std::complex<double> z) {
        // Iterate dz
        dz = 2.0 * dz * ref[ref_n] + dz * dz + dc;
        ++ref_n;
        z = ref[ref_n] + dz;

        // Rebase
        if (square_magnitude(z) < square_magnitude(dz) || ref_n >= ref.size()) {
            dz = z;
            ref_n = 0;
        }
        return z;
    });
}
auto compute_reference(multi_complex c, unsigned int max_n) -> std::vector<std::complex<double>> {
    std::vector<std::complex<double>> reference { {0.0, 0.0} };

    multi_complex z {0.0, 0.0};
    for (auto n {0u}; n < max_n && !escaped(z); ++n) {
        z = compute_next_z(z, c);
        reference.emplace_back(static_cast<std::complex<double>>(z));
    }

    return reference;
}

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

}   // namespace wacfrac
