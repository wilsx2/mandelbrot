#pragma once

#include "wacfrac/bla.hpp"
#include "wacfrac/complex_concept.hpp"
#include "wacfrac/types.hpp"
#include <wacfrac/color.hpp>
#include <wacfrac/log.hpp>
#include <wacfrac/orbit.hpp>
#include <wacfrac/resolution.hpp>

namespace wacfrac
{

template <ComplexConcept T>
auto get_pixel_delta(const MultiComplex& dimensions, const Resolution& res) -> T {
    using CT = ComplexValueTypeT<T>;
    return {
        to_real<CT>(dimensions.real()) / static_cast<CT>(res.width),
        to_real<CT>(dimensions.imag()) / static_cast<CT>(res.height)
    };
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
