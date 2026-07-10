#pragma once

#include <vector>
#include <wacfrac/types.hpp>
#include <wacfrac/resolution.hpp>
#include <wacfrac/complex_concept.hpp>
#include <wacfrac/log.hpp>
#include <wacfrac/reference.hpp>
#include <wacfrac/analysis.hpp>
#include <boost/multiprecision/detail/default_ops.hpp>
#include <cstddef>
#include <limits>

namespace wacfrac
{

struct Viewport {
    MultiComplex center;
    MultiComplex dimensions;

    Viewport() = default;
    Viewport(const MultiComplex& center, const MultiComplex& dimensions);
    Viewport(MultiComplex&& center, MultiComplex&& dimensions);
    Viewport(const MultiComplex& center, const MultiFloat& zoom, const Resolution& res);
    void precision(std::size_t value);
    auto zoomed(MultiFloat scale) const -> Viewport;
    auto sample(std::size_t x, std::size_t y, std::size_t width, std::size_t height) const -> MultiComplex;
    auto compute_max_dc(MultiComplex c) const -> MultiComplex;
    auto required_precision() const -> std::size_t;
    auto required_iterations(double modifier = 250.0, double factor = 50.0, double exponent = 1.5) const -> unsigned;
    template<ComplexConcept T>
    auto generate_probes(std::size_t cols, std::size_t rows) const -> std::vector<T>;
    template<ComplexConcept T>
    auto find_periodic_reference(unsigned max_n, unsigned find_nucleus_iter) const -> std::pair<MultiComplex, std::vector<T>>;
};
auto required_precision(MultiFloat zoom_factor) -> std::size_t;
auto required_iterations(MultiFloat zoom_factor, double modifier = 250.0, double factor = 50.0, double exponent = 1.5) -> unsigned;

template<ComplexConcept T>
auto Viewport::generate_probes(std::size_t cols, std::size_t rows) const -> std::vector<T> {
    using CT = ComplexValueTypeT<T>;
    std::vector<T> probes;
    T dimensions_t {
        to_real<CT>(dimensions.real()),
        to_real<CT>(dimensions.imag())
    };
    T interval {
        dimensions_t.real() / CT(cols),
        dimensions_t.imag() / CT(rows)
    };
    if (interval.real() == CT(0) || interval.imag() == CT(0)) {
        probes.emplace_back(CT(0), CT(0));
        return probes;
    }
    T start {
        cols % 2 == 0 ? -dimensions_t.real() / CT(2) : (-dimensions_t.real() + interval.real()) / CT(2),
        rows % 2 == 0 ? -dimensions_t.imag() / CT(2) : (-dimensions_t.imag() + interval.imag()) / CT(2)
    };
    T end {
        +dimensions_t.real() / CT(2),
        +dimensions_t.imag() / CT(2)
    };
    for (CT dx = start.real(); dx <= end.real(); dx += interval.real()) {
        for (CT dy = start.imag(); dy <= end.imag(); dy += interval.imag()) {
            probes.emplace_back(dx, dy);
        }
    }
    return probes;
}

// https://philthompson.me/2023/Faster-Mandelbrot-Set-Rendering-with-BLA-Bivariate-Linear-Approximation.html
template<ComplexConcept T>
auto Viewport::find_periodic_reference(unsigned max_n, unsigned find_nucleus_iter) const -> std::pair<MultiComplex, std::vector<T>> {
    using boost::multiprecision::isnan;
    logging::info( "Searching for periodic reference (max_n={}, nucleus_iter={})", max_n, find_nucleus_iter);

    auto half_dx = dimensions.real() / 2.0;
    auto half_dy = dimensions.imag() / 2.0;

    auto inf {std::numeric_limits<double>::infinity()};
    // Find first non-degenerate reference
    std::size_t period;
    PeriodFinder iter {center, half_dx, half_dy, max_n};
    while ((period = iter.next()) != 0) {
        logging::debug( "Trying period {} for non-degenerate reference", period);
        auto c_ref {find_nucleus(center, period, find_nucleus_iter)};
        if (isnan(c_ref.real()) || isnan(c_ref.imag())) {
            logging::debug( "Failed to find nucleus");
            continue;
        }
        logging::debug( "Nucleus found, starting on reference");
        auto reference {compute_reference_mt<T>(c_ref, period, inf)};
        if (!is_reference_degenerate(reference)) {
            logging::debug( "Found non-degenerate reference at period {}", period);
            return std::make_pair(c_ref, reference);
        }
    }

    // Fallback to center if none found
    logging::warning( "No suitable periodic reference found, falling back to view center");
    return std::make_pair(
        center,
        compute_reference_mt<T>(center, max_n, inf)
    );
}

}   // namespace wacfrac
