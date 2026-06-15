#include <wacfrac/plot.hpp>
#include <wacfrac/approximation.hpp>
#include <cmath>
#include <ranges>
#include <print>

namespace wacfrac
{

auto render_directly(const plot& p, const std::span<pixel>& buffer) -> bool {
    if (p.res.area() != buffer.size())
        return false;

    auto i {0uz};
    for (auto [y, x] : p.res.coordinates()) {
        auto c = p.view.sample(x, y, p.res.width, p.res.height);
        auto n = escape_time<orbit<std::complex<double>>>({c.convert_to<double>()}, p.max_iterations);
        auto percent = n / static_cast<float>(p.max_iterations);
        buffer[i++] = (percent == 1.f) ? pixel(0,0,0) : hsv_to_rgb(percent,1.f,1.f);
    }

    return true;
}

auto render_perturbed(const plot& p, const std::span<pixel>& buffer) -> bool {
    if (p.res.area() != buffer.size())
        return false;

    auto c_ref   = p.view.center;
    std::vector<std::complex<double>> reference = compute_reference(c_ref, p.max_iterations);

    auto i {0uz};
    for (auto [y, x] : p.res.coordinates()) {
        auto c_pixel = p.view.sample(x, y, p.res.width, p.res.height);
        std::complex<double> dc {
            static_cast<double>(c_pixel.real() - c_ref.real()),
            static_cast<double>(c_pixel.imag() - c_ref.imag())
        };
        auto n = escape_time<perturbed_orbit>({reference, dc}, p.max_iterations);
        auto percent = n / static_cast<float>(p.max_iterations);
        buffer[i++] = (percent == 1.f) ? pixel(0,0,0) : hsv_to_rgb(percent,1.f,1.f);
    }
    return true;
}

auto render_approximated(const plot& p, const std::span<pixel>& buffer) -> bool {

        if (p.res.area() != buffer.size())
        return false;

    auto c_ref   = p.view.center;
    std::vector<std::complex<double>> reference = compute_reference(c_ref, p.max_iterations);
    series_approximator sa {reference, 12}; // TODO: Replace magic number
    sa.compute_coeffs_while_valid({static_cast<std::complex<double>>(c_ref)}, 1e-6); // TODO: Replace magic number; use many probes

    auto i {0uz};
    for (auto [y, x] : p.res.coordinates()) {
        auto c_pixel = p.view.sample(x, y, p.res.width, p.res.height);
        std::complex<double> dc {
            static_cast<double>(c_pixel.real() - c_ref.real()),
            static_cast<double>(c_pixel.imag() - c_ref.imag())
        };
        auto dz = sa.approximate_delta_n(dc);
        auto n = escape_time<perturbed_orbit>({reference, dz, dc}, p.max_iterations, sa.n());
        auto percent = n / static_cast<float>(p.max_iterations);
        buffer[i++] = (percent == 1.f) ? pixel(0,0,0) : hsv_to_rgb(percent,1.f,1.f);
    }
    return true;
}

}   // namespace wacfrac
