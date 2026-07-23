#pragma once

#include "wacfrac/bla.hpp"
#include "wacfrac/complex_concept.hpp"
#include "wacfrac/types.hpp"
#include <wacfrac/color.hpp>
#include <wacfrac/log.hpp>
#include <wacfrac/orbit.hpp>
#include <wacfrac/resolution.hpp>
#include <sycl/sycl.hpp>

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

template <ComplexConcept T>
SYCL_EXTERNAL
auto sample_c_value(sycl::id<2> id,
                    T start,
                    T delta) -> T {
    using CT = ComplexValueTypeT<T>;
    return T {
        start.real() + delta.real() * static_cast<CT>(id.get(1)), // col 
        start.imag() + delta.imag() * static_cast<CT>(id.get(0))  // row
    };
}

}   // namespace wacfrac
