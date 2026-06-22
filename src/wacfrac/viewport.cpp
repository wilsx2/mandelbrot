#include <wacfrac/viewport.tpp>
#include "wacfrac/types.hpp"
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
    auto re = (multi_float(x, dimensions.precision()) / multi_float(width, dimensions.precision())  - 0.5) * dimensions.real() + center.real();
    auto im = (multi_float(y, dimensions.precision()) / multi_float(height, dimensions.precision()) - 0.5) * dimensions.imag() + center.imag();
    return multi_complex(re, im);
}

template auto viewport::generate_probes<std::complex<double>>(std::size_t cols, std::size_t rows) const -> std::vector<std::complex<double>>;
template auto viewport::generate_probes<std::complex<long double>>(std::size_t cols, std::size_t rows) const -> std::vector<std::complex<long double>>;
template auto viewport::generate_probes<doubleexp_complex>(std::size_t cols, std::size_t rows) const -> std::vector<doubleexp_complex>;

auto viewport::required_precision() const -> std::size_t {
    auto zoom = static_cast<double>(dimensions.real());
    if (zoom <= 0.0) {
        auto mz = dimensions.real();
        return static_cast<std::size_t>(-boost::multiprecision::log10(mz) + 4);
    }
    return static_cast<std::size_t>(std::log10(1.0 / zoom) + 4.0);
}

auto viewport::required_iterations(double modifier, double factor, double exponent) const -> std::size_t {
    auto zoom = static_cast<double>(dimensions.real());
    if (zoom > 1.0)
        return modifier;
    if (zoom <= 0.0) {
        auto mz = dimensions.real();
        return modifier + factor * std::pow(static_cast<double>(-boost::multiprecision::log10(mz)), exponent);
    }
    return modifier + factor * std::pow(std::log10(1.0 / zoom), exponent);
}

}   // namespace wacfrac
