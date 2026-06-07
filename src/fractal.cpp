#include <wacfrac/fractal.hpp>

namespace wacfrac
{

auto escape_time(multi_complex c, unsigned int max_iterations) -> unsigned int {
    multi_complex z {0.0, 0.0};
    auto n = 0u;

    while (z.real()*z.real() + z.imag()*z.imag() < 4 && n < max_iterations) {
        z = z*z + c;
        ++n;
    }

    return n;
}

}   // namespace wacfrac
