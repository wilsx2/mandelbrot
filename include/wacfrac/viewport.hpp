#pragma once

#include "wacfrac/rendering.hpp"
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
    auto get_corner_absolute() const -> T;
    template<ComplexConcept T>
    auto get_corner_relative() const -> T;
    template<ComplexConcept T>
    auto generate_probes(std::size_t cols, std::size_t rows) const -> std::vector<T>;
    template<ComplexConcept T>
    auto find_periodic_reference(unsigned max_n, unsigned find_nucleus_iter) const -> std::pair<MultiComplex, std::vector<T>>;
};
auto required_precision(MultiFloat zoom_factor) -> std::size_t;
auto required_iterations(MultiFloat zoom_factor, double modifier = 250.0, double factor = 50.0, double exponent = 1.5) -> unsigned;

template<ComplexConcept T>
auto Viewport::get_corner_absolute() const -> T {
    using CT = ComplexValueTypeT<T>;
    return {
        to_real<CT>(center.real()) - to_real<CT>(dimensions.real()) / static_cast<CT>(2.0),
        to_real<CT>(center.imag()) - to_real<CT>(dimensions.imag()) / static_cast<CT>(2.0)
    };
}
template<ComplexConcept T>
auto Viewport::get_corner_relative() const -> T {
    using CT = ComplexValueTypeT<T>;
    return {
        -to_real<CT>(dimensions.real()) / static_cast<CT>(2.0),
        -to_real<CT>(dimensions.imag()) / static_cast<CT>(2.0)
    };
}

template<ComplexConcept T>
auto Viewport::generate_probes(std::size_t cols, std::size_t rows) const -> std::vector<T> {
    using CT = ComplexValueTypeT<T>;
    std::vector<T> probes; // WARN: Should just fill up a span passed as an argument
    auto delta {get_pixel_delta<T>(dimensions, Resolution{cols, rows})};
    auto corner {get_corner_relative<T>()};
    if (cols % 2)
        corner.real() += delta.real() / static_cast<CT>(2.0);
    if (rows % 2)
        corner.imag() += delta.imag() / static_cast<CT>(2.0);
    for (auto col {0u}; col < cols; ++col) { // TODO: Cartesian product view. Parallel for if we're feeling fun
        for (auto row {0u}; row < rows; ++row) {
            probes.emplace_back(
                corner.real() + delta.real() * static_cast<CT>(col),
                corner.imag() + delta.imag() * static_cast<CT>(row));
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
