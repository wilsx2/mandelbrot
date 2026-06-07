#pragma once

#include <wacfrac/types.hpp>

namespace wacfrac
{

template<typename T>
    // TODO: Concepts
auto escape_time(T c, unsigned int max_iterations) -> unsigned int {
    T z {0.0, 0.0};
    auto n = 0u;

    while (z.real()*z.real() + z.imag()*z.imag() < 4 && n < max_iterations) {
        z = z*z + c;
        ++n;
    }

    return n;
}

}   // namespace wacfrac
