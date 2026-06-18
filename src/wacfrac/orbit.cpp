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

}   // namespace wacfrac
