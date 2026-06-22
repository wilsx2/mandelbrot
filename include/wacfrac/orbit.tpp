#pragma once

#include <wacfrac/orbit.hpp>
#include <complex>
#include <tuple>
#include <utility>
#include <vector>

namespace wacfrac
{

template <Complex T>
auto rebase_reference(const std::vector<T>& ref, std::size_t ref_n, T dz) -> std::tuple<std::size_t, T, T> {
    if (ref_n >= ref.size()) {
        return {0uz, dz, dz};
    }

    auto z {ref[ref_n] + dz};

    using std::norm;
    if (norm(z) < norm(dz)) {
        dz = z;
        ref_n = 0;
    }
    return {ref_n, dz, z};
}

template <Complex T>
auto compute_next_perturbation(const std::vector<T>& ref, std::size_t ref_n, T dc, T dz) -> std::tuple<std::size_t, T, T> {
    dz = T{2.0, 0.0} * dz * ref[ref_n] + dz * dz + dc;
    ++ref_n;
    return rebase_reference(ref, ref_n, dz);
}

template <Complex T>
auto escape_perturbed(const std::vector<T>& ref, T dc, std::size_t max_n, T dz, std::size_t n) -> std::pair<T, std::size_t> {
    auto ref_n {n};
    if (ref_n >= ref.size()) {
        return {{}, 0};
    }

    T z0 {ref[ref_n] + dz};
    return escape_generic(z0, n, max_n, [&](T z) {
        std::tie(ref_n, dz, z) = compute_next_perturbation(ref, ref_n, dc, dz);
        return z;
    });
}

template <Complex T>
auto compute_reference(multi_complex c, std::size_t max_n, bool do_escape) -> std::vector<T> {
    std::vector<T> reference { T{0.0, 0.0} };

    multi_complex z {0.0, 0.0};
    for (auto n {0uz}; n < max_n - 1 && (!do_escape || !escaped(z)); ++n) {
        z = compute_next_z(z, c);
        using CT = complex_value_type_t<T>;
        reference.emplace_back(
            static_cast<CT>(boost::multiprecision::real(z)),
            static_cast<CT>(boost::multiprecision::imag(z))
        );
    }

    return reference;
}

}   // namespace wacfrac
