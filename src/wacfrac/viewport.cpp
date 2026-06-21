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
    auto re = (multi_float(x, dimensions.precision()) / multi_float(width, dimensions.precision())  - 0.5) * dimensions.real() + center.real();
    auto im = (multi_float(y, dimensions.precision()) / multi_float(height, dimensions.precision()) - 0.5) * dimensions.imag() + center.imag();
    return multi_complex(re, im);
}

auto viewport::generate_probes(std::size_t cols, std::size_t rows) const -> std::vector<std::complex<double>> {
    std::vector<std::complex<double>> probes;
    std::complex<double> interval {
        static_cast<double>(dimensions.real()/cols),
        static_cast<double>(dimensions.imag()/rows)
    };
    if (interval.real() == 0.0 || interval.imag() == 0.0) {
        probes.emplace_back(0.0, 0.0);
        return probes;
    }
    std::complex<double> start {
        static_cast<double>(cols % 2 == 0 ? -dimensions.real()/2.0 : (-dimensions.real() + interval.real())/2.0),
        static_cast<double>(rows % 2 == 0 ? -dimensions.imag()/2.0 : (-dimensions.imag() + interval.imag())/2.0)
    };
    std::complex<double> end {
        static_cast<double>(+dimensions.real()/2.0),
        static_cast<double>(+dimensions.imag()/2.0)
    };
    for (double dx = start.real(); dx <= end.real(); dx += interval.real()) {
        for (double dy = start.imag(); dy <= end.imag(); dy += interval.imag()) {
            probes.emplace_back(dx, dy);
        }
    }
    return probes;
}

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
