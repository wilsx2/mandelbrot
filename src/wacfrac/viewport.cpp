#include <wacfrac/viewport.hpp>

namespace wacfrac
{

void viewport::precision(std::size_t value) {
    min.precision(value);
    max.precision(value);
}

auto viewport::at_zoom(multi_complex focus, multi_float factor) -> viewport {
    auto width  = max.real() - min.real();
    auto height = max.imag() - min.imag();

    auto focus_re = (focus.real() - min.real()) / width;
    auto focus_im = (focus.imag() - min.imag()) / height;

    auto new_width  = width  / factor;
    auto new_height = height / factor;

    auto new_min = multi_complex(
        focus.real() - focus_re * new_width,
        focus.imag() - focus_im * new_height
    );

    return { new_min, new_min + multi_complex(new_width, new_height) };
}

auto viewport::sample(std::size_t x, std::size_t y, std::size_t width, std::size_t height) const -> multi_complex {
    auto re = multi_float(x) / multi_float(width)  * (max.real() - min.real()) + min.real();
    auto im = multi_float(y) / multi_float(height) * (max.imag() - min.imag()) + min.imag();
    return multi_complex(re, im);
}

}   // namespace wacfrac
