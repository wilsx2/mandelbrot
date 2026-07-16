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
    template<ComplexConcept T, typename Context>
    auto generate_probes(const Context& ctx, WF_STD::span<T> buffer, std::size_t cols, std::size_t rows) const -> void;
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

template<ComplexConcept T, typename Context>
auto Viewport::generate_probes(const Context& ctx, WF_STD::span<T> buffer, std::size_t cols, std::size_t rows) const -> void {
    using CT = ComplexValueTypeT<T>;
    auto delta {get_pixel_delta<T>(dimensions, Resolution{cols, rows})};
    auto corner {get_corner_relative<T>()};
    if (cols % 2)
        corner.real() += delta.real() / static_cast<CT>(2.0);
    if (rows % 2)
        corner.imag() += delta.imag() / static_cast<CT>(2.0);
    ctx.parallel_for(rows*cols,
        [cols, buffer, corner, delta]
        WF_HD
        (int tid){
        auto col {tid % cols};
        auto row {tid / cols};
        buffer[tid] = T{
            corner.real() + delta.real() * static_cast<CT>(col),
            corner.imag() + delta.imag() * static_cast<CT>(row)
        };
    });
}

}   // namespace wacfrac
