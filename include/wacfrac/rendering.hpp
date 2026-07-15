#pragma once

#include "wacfrac/bla.hpp"
#include "wacfrac/io.hpp"
#include "wacfrac/types.hpp"
#include <wacfrac/color.hpp>
#include <wacfrac/log.hpp>
#include <wacfrac/orbit.hpp>
#include <wacfrac/viewport.hpp>
#include <wacfrac/resolution.hpp>
#include <ranges>

namespace wacfrac
{

template <ComplexConcept T>
auto sample_c_values(const Viewport& view, const Resolution& res, T center = 0.0) {
    using CT = ComplexValueTypeT<T>;
    auto start {to_complex<T>(view.sample(0, 0, res.width, res.height)) - T(center)};
    auto delta_real {to_real<CT>(view.dimensions.real() / MultiFloat(res.width))};
    auto delta_imag {to_real<CT>(view.dimensions.imag() / MultiFloat(res.height))};
    return res.coordinates() | std::views::transform([start, delta_real, delta_imag](auto coord){
        auto [y, x] = coord;
        return start + T(
            static_cast<CT>(x) * delta_real,
            static_cast<CT>(y) * delta_imag
        );
    });
}

template <ComplexConcept T>
WF_HD
auto sample_c_value(std::size_t idx,
                    std::size_t row_width,
                    T start,
                    T delta) -> T {
    using CT = ComplexValueTypeT<T>;
    CT x {static_cast<CT>(idx % row_width)};
    CT y {static_cast<CT>(idx / row_width)};
    return T {
        start.real() + delta.real() * x,
        start.imag() + delta.imag() * y
    };
}

}   // namespace wacfrac
