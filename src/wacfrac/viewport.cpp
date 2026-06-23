#include "wacfrac/viewport.hpp"
#include <complex>
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

auto viewport::compute_max_dc(multi_complex c) const -> multi_complex {
    multi_complex c_tl {center - dimensions / 2.0};
    multi_complex c_tr {
        center.real() + dimensions.real() / 2.0,
        center.imag() - dimensions.imag() / 2.0
    };
    multi_complex c_br {center + dimensions / 2.0};
    multi_complex c_bl {
        center.real() - dimensions.real() / 2.0,
        center.imag() + dimensions.imag() / 2.0
    };

    using boost::multiprecision::abs;
    auto max_dc {multi_complex{c_tl - c}};
    if (abs(multi_complex(c_tr - c)) > abs(max_dc)) max_dc = multi_complex(c_tr - c);
    if (abs(multi_complex(c_br - c)) > abs(max_dc)) max_dc = multi_complex(c_br - c);
    if (abs(multi_complex(c_bl - c)) > abs(max_dc)) max_dc = multi_complex(c_bl - c);
    return max_dc;
}

auto viewport::required_precision() const -> std::size_t {
    return wacfrac::required_precision(dimensions.real());
}

auto viewport::required_iterations(double modifier, double factor, double exponent) const -> std::size_t {
    return wacfrac::required_iterations(dimensions.real(), modifier, factor, exponent);
}

template auto viewport::generate_probes<std::complex<double>>(std::size_t cols, std::size_t rows) const -> std::vector<std::complex<double>>;
template auto viewport::generate_probes<std::complex<long double>>(std::size_t cols, std::size_t rows) const -> std::vector<std::complex<long double>>;
template auto viewport::generate_probes<doubleexp_complex>(std::size_t cols, std::size_t rows) const -> std::vector<doubleexp_complex>;

template auto viewport::find_periodic_reference<std::complex<double>>(std::size_t max_n, std::size_t find_period_iter, std::size_t find_nucleus_iter) const -> std::pair<multi_complex, std::vector<std::complex<double>>>;
template auto viewport::find_periodic_reference<std::complex<long double>>(std::size_t max_n, std::size_t find_period_iter, std::size_t find_nucleus_iter) const -> std::pair<multi_complex, std::vector<std::complex<long double>>>;
template auto viewport::find_periodic_reference<doubleexp_complex>(std::size_t max_n, std::size_t find_period_iter, std::size_t find_nucleus_iter) const -> std::pair<multi_complex, std::vector<doubleexp_complex>>;

auto required_precision(multi_float zoom) -> std::size_t {
    double trunc_zoom {zoom};
    if (trunc_zoom <= 0.0) {
        return static_cast<std::size_t>(-boost::multiprecision::log10(zoom) + 4);
    }
    return static_cast<std::size_t>(std::log10(1.0 / trunc_zoom) + 4.0);
}
auto required_iterations(multi_float zoom, double modifier, double factor, double exponent) -> std::size_t {
    double trunc_zoom {zoom};
    if (trunc_zoom > 1.0)
        return modifier;
    if (trunc_zoom <= 0.0) {
        return modifier + factor * std::pow(static_cast<double>(-boost::multiprecision::log10(zoom)), exponent);
    }
    return modifier + factor * std::pow(std::log10(1.0 / trunc_zoom), exponent);
}

}   // namespace wacfrac
