#pragma once

#include "wacfrac/complex_concept.hpp"
#include "wacfrac/orbit.hpp"
#include "wacfrac/types.hpp"
#include "wacfrac/log.hpp"
#include <vector>
#include <barrier>
#include <thread>

namespace wacfrac {

template <ComplexConcept T = DoubleComplex>
auto compute_reference(MultiComplex c, unsigned max_n, double escape_radius = 4.0) -> std::vector<T> {
    logging::debug( "Computing reference orbit at ({}) max_n={} escape_radius={}", c, max_n, escape_radius);

    std::vector<T> reference;
    reference.reserve(max_n);
    reference.emplace_back(to_complex<T>(MultiComplex{0.0, 0.0}));

    MultiComplex z{0.0, 0.0};
    for (auto n{0u}; n < max_n - 1 && !escaped(z, escape_radius);) {
        compute_next_orbit(z, n, c);
        reference.emplace_back(to_complex<T>(z));
    }

    logging::debug( "Reference orbit computed: {} points (max_n={})", reference.size(), max_n);
    return reference;
}

template <ComplexConcept T = DoubleComplex>
auto compute_reference_mt(MultiComplex c, unsigned max_n, double escape_radius = 4.0) -> std::vector<T> {
    logging::debug( "Computing reference orbit at ({}) max_n={} escape_radius={} (parallel)", c, max_n, escape_radius);
    if (max_n == 0)
        return {};

    std::vector<T> reference (max_n);
    reference[0] = T{0.0, 0.0};
    using CT = ComplexValueTypeT<T>;

    auto count = compute_reference_iteration(c, max_n, escape_radius, [&](unsigned n, const MultiFloat& r, const MultiFloat& i) {
        reference[n] = T{to_real<CT>(r), to_real<CT>(i)};
    });

    reference.resize(count);
    logging::debug( "Reference orbit computed: {} points (max_n={})", reference.size(), max_n);
    return reference;
}

template <std::invocable<unsigned, const MultiFloat&, const MultiFloat&> F>
auto compute_reference_iteration(MultiComplex c, unsigned max_n, double escape_radius, F&& store_at_n) -> unsigned {
    if (max_n == 0)
        return 0;

    auto n_1 {1u};
    MultiComplex z {0.0, 0.0};
    MultiComplex next_z {0.0, 0.0};
    auto running {true};
    auto cr {c.real()};
    auto ci {c.imag()};
    MultiFloat computed_re;
    MultiFloat computed_im;

    std::barrier sync (2, [&]() noexcept {
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

    real_compute.join();
    imag_compute.join();
    return n_1;
}

inline auto compute_reference_set(MultiComplex c, unsigned max_n, double escape_radius = 4.0) -> ReferenceSet {
    ReferenceSet refs;
    if (max_n == 0)
        return refs;

    refs.float_ref.resize(max_n);
    refs.double_ref.resize(max_n);
    refs.dexp_ref.resize(max_n);
    refs.float_ref[0]       = SingleComplex{0.0f, 0.0f};
    refs.double_ref[0]      = DoubleComplex{0.0, 0.0};
    refs.dexp_ref[0]        = to_complex<DoubleExpComplex>(MultiComplex{0.0, 0.0});

    auto count = compute_reference_iteration(c, max_n, escape_radius, [&](unsigned n, const MultiFloat& r, const MultiFloat& i) {
        refs.float_ref[n]       = SingleComplex{to_real<float>(r), to_real<float>(i)};
        refs.double_ref[n]      = DoubleComplex{to_real<double>(r), to_real<double>(i)};
        refs.dexp_ref[n]        = DoubleExpComplex{to_real<DoubleExp>(r), to_real<DoubleExp>(i)};
    });

    refs.float_ref.resize(count);
    refs.double_ref.resize(count);
    refs.dexp_ref.resize(count);

    logging::debug( "All references computed: {} points", count);
    return refs;
}

} // namespace wacfrac
