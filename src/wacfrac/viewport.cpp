#include <wacfrac/viewport.hpp>
#include <cmath>

namespace wacfrac
{

void viewport::precision(std::size_t value) {
    center.precision(value);
    dimensions.precision(value);
}

auto viewport::zoomed(multi_float factor) const -> viewport {
    return {center, dimensions / factor};
}

auto viewport::sample(std::size_t x, std::size_t y, std::size_t width, std::size_t height) const -> multi_complex {
    // NOTE: multi_float precision unset
    auto re = (multi_float(x, dimensions.precision()) / multi_float(width, dimensions.precision())  - 0.5) * dimensions.real() + center.real();
    auto im = (multi_float(y, dimensions.precision()) / multi_float(height, dimensions.precision()) - 0.5) * dimensions.imag() + center.imag();
    return multi_complex(re, im);
}


auto approximate_required_precision(multi_float scale) -> unsigned int {
    return std::log10(scale.convert_to<double>()) + 4.0; // TODO: Use proper member overloads
}
auto approximate_required_iterations(multi_float scale, double modifier, double factor, double exponent) -> unsigned int {
    if (scale < 1.0)
        return modifier;
    return modifier + factor * std::pow(std::log10(scale.convert_to<double>()), exponent);
}


}   // namespace wacfrac
