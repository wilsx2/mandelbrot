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

template <std::invocable<std::size_t, const MultiFloat&, const MultiFloat&> F>
auto compute_reference_iteration(MultiComplex c, std::size_t max_n, double escape_radius, F&& store_at_n) -> std::size_t {
    if (max_n == 0)
        return 0;

    auto n_1 {1uz};
    MultiComplex z {0.0, 0.0};
    MultiComplex next_z {0.0, 0.0};
    auto running {true};
    auto cr {c.real()};
    auto ci {c.imag()};
    MultiFloat computed_re;
    MultiFloat computed_im;

    std::barrier sync (2, [&](){
        store_at_n(n_1, computed_re, computed_im);
        ++n_1;
        if (n_1 >= max_n || escaped(next_z, escape_radius))
            running = false;
        else
            std::swap(z, next_z);
    });

    std::jthread real_compute {[&](){
        while (running) {
            computed_re = z.real() * z.real() - z.imag() * z.imag() + cr;
            next_z.real(computed_re);
            sync.arrive_and_wait();
        }
    }};
    std::jthread imag_compute {[&](){
        while (running) {
            computed_im = 2 * z.real() * z.imag() + ci;
            next_z.imag(computed_im);
            sync.arrive_and_wait();
        }
    }};

    return n_1;
}

template <Complex T>
auto compute_reference_mt(MultiComplex c, std::size_t max_n, double escape_radius) -> std::vector<T> {
    logging::print(logging::Severity::Debug, "Computing reference orbit at ({}) max_n={} escape_radius={} (parallel)", c, max_n, escape_radius);
    if (max_n == 0)
        return {};

    std::vector<T> reference (max_n);
    reference[0] = T{0.0, 0.0};
    using CT = ComplexValueTypeT<T>;

    auto count = compute_reference_iteration(c, max_n, escape_radius, [&](std::size_t n, const MultiFloat& r, const MultiFloat& i) {
        reference[n] = T{to_real<CT>(r), to_real<CT>(i)};
    });

    reference.resize(count);
    logging::print(logging::Severity::Debug, "Reference orbit computed: {} points (max_n={})", reference.size(), max_n);
    return reference;
}

auto compute_references_all(MultiComplex c, std::size_t max_n, double escape_radius) -> ReferenceSet {
    ReferenceSet refs;
    if (max_n == 0)
        return refs;

    refs.double_ref.resize(max_n);
    refs.long_double_ref.resize(max_n);
    refs.dexp_ref.resize(max_n);
    refs.double_ref[0]      = std::complex<double>{0.0, 0.0};
    refs.long_double_ref[0] = std::complex<long double>{0.0, 0.0};
    refs.dexp_ref[0]        = to_complex<DoubleExpComplex>(MultiComplex{0.0, 0.0});

    auto count = compute_reference_iteration(c, max_n, escape_radius, [&](std::size_t n, const MultiFloat& r, const MultiFloat& i) {
        refs.double_ref[n]      = std::complex<double>{to_real<double>(r), to_real<double>(i)};
        refs.long_double_ref[n] = std::complex<long double>{to_real<long double>(r), to_real<long double>(i)};
        refs.dexp_ref[n]        = DoubleExpComplex{to_real<DoubleExp>(r), to_real<DoubleExp>(i)};
    });

    refs.double_ref.resize(count);
    refs.long_double_ref.resize(count);
    refs.dexp_ref.resize(count);

    logging::print(logging::Severity::Debug, "All references computed: {} points", count);
    return refs;
}

}   // namespace wacfrac
