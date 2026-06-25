#pragma once

#include <wacfrac/color.hpp>
#include <wacfrac/log.hpp>
#include <wacfrac/orbit.hpp>
#include <wacfrac/viewport.hpp>
#include <wacfrac/resolution.hpp>
#include <execution>
#include <span>
#include <cstddef>

namespace wacfrac
{

template <std::invocable<std::size_t, std::size_t> F, typename G>
void screen_render(std::span<pixel> pixels, const resolution& res, F&& escape_fn, G&& color_fn) {
    LOG_DEBUG << "Screen render: " << res.width << "x" << res.height
              << " (" << res.area() << " pixels, parallel)";
    auto coords {res.coordinates()};
    std::for_each(std::execution::par, coords.begin(), coords.end(),[&](auto&& coord){
        auto [y, x] = coord;
        auto result = escape_fn(x, y);

        std::size_t n {std::get<1>(result)};
        pixels[x + y * res.width] = [&]() {
            if constexpr (std::invocable<G, std::complex<float>, decltype(n)>) {
                std::complex<float> z {std::get<0>(result)};
                return color_fn(z, n);
            } else if constexpr (std::invocable<G, decltype(n)>) {
                return color_fn(n);
            } else {
                static_assert(false, "color_fn does not accept (z, n) or (n)");
            }
        }();
    });
}

template <Complex T, std::invocable<T> F, typename G>
void absolute_render(std::span<pixel> pixels, const resolution& res, const viewport& view, F&& escape_fn, G&& color_fn) {
    using CT = complex_value_type_t<T>;

    auto c_start = to_complex<T>(view.sample(0, 0, res.width, res.height));
    auto d_re = to_real<CT>(view.dimensions.real() / multi_float(res.width));
    auto d_im = to_real<CT>(view.dimensions.imag() / multi_float(res.height));

    screen_render(pixels, res,
        [c_start, d_re, d_im, &escape_fn](std::size_t x, std::size_t y) {
            T c = c_start + T(static_cast<CT>(x) * d_re, static_cast<CT>(y) * d_im);
            return escape_fn(c);
        },
        color_fn
    );
}

template <Complex T, std::invocable<T> F, typename G>
void perturbed_render(std::span<pixel> pixels, const resolution& res, const viewport& view, F&& escape_fn, G&& color_fn, multi_complex ref_center) {
    using CT = complex_value_type_t<T>;

    auto dc_start = to_complex<T>(view.sample(0, 0, res.width, res.height) - ref_center);
    auto d_re = to_real<CT>(view.dimensions.real() / multi_float(res.width));
    auto d_im = to_real<CT>(view.dimensions.imag() / multi_float(res.height));

    screen_render(pixels, res,
        [dc_start, d_re, d_im, &escape_fn](std::size_t x, std::size_t y) {
            T dc = dc_start + T(static_cast<CT>(x) * d_re, static_cast<CT>(y) * d_im);
            return escape_fn(dc);
        },
        color_fn
    );
}

}   // namespace wacfrac
