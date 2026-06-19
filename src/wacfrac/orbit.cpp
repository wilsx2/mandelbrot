#include <wacfrac/orbit.hpp>
#include <complex>
#include <tuple>
#include <utility>
#include <vector>

namespace wacfrac
{

auto rebase_reference(const std::vector<std::complex<double>>& ref, std::size_t ref_n, std::complex<double> dz) -> std::tuple<std::size_t, std::complex<double>, std::complex<double>> {
    if (ref_n >= ref.size()) {
        return {0uz, dz, dz};
    }

    auto z {ref[ref_n] + dz};

    if (std::norm(z) < std::norm(dz)) {
        dz = z;
        ref_n = 0;
    }
    return {ref_n, dz, z};
}

auto compute_next_perturbation(const std::vector<std::complex<double>>& ref, std::size_t ref_n, std::complex<double> dc, std::complex<double> dz) -> std::tuple<std::size_t, std::complex<double>, std::complex<double>> {
    dz = 2.0 * dz * ref[ref_n] + dz * dz + dc;
    ++ref_n;
    return rebase_reference(ref, ref_n, dz);
}

// https://fractalforums.org/index.php?topic=4360.0
auto escape_perturbed(const std::vector<std::complex<double>>& ref, std::complex<double> dc, unsigned int max_n, std::complex<double> dz, unsigned int n) -> std::pair<std::complex<double>, unsigned int> {
    auto ref_n {n};
    if (ref_n >= ref.size()) {
        return {{}, 0};
    }

    auto z0 {ref[ref_n] + dz};
    return escape_generic(z0, n, max_n, [&](std::complex<double> z) {
        std::tie(ref_n, dz, z) = compute_next_perturbation(ref, ref_n, dc, dz);
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
