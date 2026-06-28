#pragma once

#include "wacfrac/types.hpp"
#include <barrier>
#include <thread>
#include <wacfrac/orbit.hpp>
#include <wacfrac/log.hpp>
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
    static const T TWO{2.0, 0.0};
    dz = TWO * dz * ref[ref_n] + dz * dz + dc;
    ++ref_n;
    return rebase_reference(ref, ref_n, dz);
}

template <Complex T>
auto escape_perturbed(const std::vector<T>& ref, T dc, std::size_t max_n, double escape_radius, T dz, std::size_t n) -> std::pair<T, std::size_t> {
    auto ref_n {n};
    if (ref_n >= ref.size()) {
        return {{}, 0};
    }

    T z0 {ref[ref_n] + dz};
    return escape_generic(z0, n, max_n, [&](T z) {
        std::tie(ref_n, dz, z) = compute_next_perturbation(ref, ref_n, dc, dz);
        return z;
    }, escape_radius);
}

template <Complex T>
auto compute_reference(MultiComplex c, std::size_t max_n, double escape_radius) -> std::vector<T> {
    logging::print(logging::Severity::Debug, "Computing reference orbit at ({}) max_n={} escape_radius={}", c, max_n, escape_radius);

    std::vector<T> reference;
    reference.reserve(max_n);
    reference.emplace_back(to_complex<T>(MultiComplex{0.0, 0.0}));

    MultiComplex z{0.0, 0.0};
    for (auto n{0uz}; n < max_n - 1 && !escaped(z, escape_radius); ++n) {
        z = z * z + c;
        reference.emplace_back(to_complex<T>(z));
    }

    logging::print(logging::Severity::Debug, "Reference orbit computed: {} points (max_n={})", reference.size(), max_n);
    return reference;
}

template <Complex T>
auto compute_reference_mt(MultiComplex c, std::size_t max_n, double escape_radius) -> std::vector<T> {
    logging::print(logging::Severity::Debug, "Computing reference orbit at ({}) max_n={} escape_radius={} (parallel)", c, max_n, escape_radius);
    if (max_n == 0)
        return {};

    std::vector<T> reference (max_n);
    reference[0] = T{0.0, 0.0};
    auto n_1 {1uz};
    MultiComplex z {0.0, 0.0};
    MultiComplex next_z {0.0, 0.0};
    auto running {true};

    auto cr {c.real()};
    auto ci {c.imag()};
    {
        std::barrier sync (2, [&](){
            if (++n_1 >= max_n)
                running = false;
            else
                std::swap(z, next_z);
        });

        std::jthread real_compute {[&](){ 
            while (running) {
                MultiFloat zr {z.real()};
                MultiFloat zi {z.imag()};
                MultiFloat nzr {zr*zr - zi*zi + cr};
                next_z.real(nzr);
                reference[n_1].real(to_real<ComplexValueTypeT<T>>(nzr));
                sync.arrive_and_wait();
            }
        }};
        std::jthread imag_compute {[&](){
            while (running) {
                MultiFloat nzi {2*z.real()*z.imag() + ci};
                next_z.imag(nzi);
                reference[n_1].imag(to_real<ComplexValueTypeT<T>>(nzi)); 
                sync.arrive_and_wait();
            }
        }};
    }

    logging::print(logging::Severity::Debug, "Reference orbit computed: {} points (max_n={})", reference.size(), max_n);
    return reference;
}

}   // namespace wacfrac
