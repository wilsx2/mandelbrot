#include <wacfrac/plot.hpp>

#include <cmath>
#include <ranges>

namespace wacfrac
{

auto render_directly (const plot& p, const std::span<pixel>& buffer) -> bool {
    if (p.res.area() != buffer.size())
        return false;

    auto i {0uz};
    for (auto [y, x] : p.res.coordinates()) {
        auto c = p.view.sample(x, y, p.res.width, p.res.height);
        auto n = escape_time(c, p.max_iterations);
        auto percent = n / static_cast<float>(p.max_iterations);
        buffer[i++] = (percent == 1.f) ? pixel(0,0,0) : hsv_to_rgb(percent,1.f,1.f);
    }

    return true;
}
auto render_perturbed(const plot& p, const std::span<pixel>& buffer) -> bool {
    if (p.res.area() != buffer.size())
        return false;

    std::vector<std::complex<double>> reference = wacfrac::calculate_orbit<std::complex<double>, wacfrac::multi_complex>((p.view.min + p.view.max)/wacfrac::multi_float(2.0), p.max_iterations);

    auto i {0uz};
    for (auto [y, x] : p.res.coordinates()) {
        auto c_pixel = p.view.sample(x, y, p.res.width, p.res.height);
        auto c_ref   = (p.view.min + p.view.max) / wacfrac::multi_float(2.0);
        std::complex<double> dc {
            static_cast<double>(c_pixel.real() - c_ref.real()),
            static_cast<double>(c_pixel.imag() - c_ref.imag())
        };
        auto n = escape_time_perturbed<std::complex<double>>(dc, reference, p.max_iterations);
        auto percent = n / static_cast<float>(p.max_iterations);
        buffer[i++] = (percent == 1.f) ? pixel(0,0,0) : hsv_to_rgb(percent,1.f,1.f);
    }
    return true;
}

}   // namespace wacfrac
