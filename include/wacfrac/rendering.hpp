#pragma once

#include <wacfrac/color.hpp>
#include <wacfrac/orbit.hpp>
#include <wacfrac/viewport.hpp>
#include <wacfrac/resolution.hpp>
#include <iostream>
#include <cstddef>

namespace wacfrac
{

template <std::invocable<std::size_t, std::size_t> F, typename G>
void screen_render(std::ostream& os, const resolution& res, F&& escape_fn, G&& color_fn) {
    for (auto [y, x] : res.coordinates()) {
        auto result = escape_fn(x, y);

        std::size_t n {std::get<1>(result)};
        auto pixel {[&]() {
            if constexpr (std::invocable<G, std::complex<float>, decltype(n)>) {
                std::complex<float> z {std::get<0>(result)};
                return color_fn(z, n);
            } else if constexpr (std::invocable<G, decltype(n)>) {
                return color_fn(n);
            } else {
                static_assert(false, "color_fn does not accept (z, n) or (n)");
            }
        }()}; 

        os.write(reinterpret_cast<const char*>(&pixel), sizeof(pixel));
    }
}

template <Complex T, std::invocable<T> F, typename G>
void absolute_render(std::ostream& os, const resolution& res, const viewport& view, F&& escape_fn, G&& color_fn) {
    screen_render(os, res,
        [&res, &view, &escape_fn](std::size_t x, std::size_t y) {
            T c (to_complex<T>(view.sample(x, y, res.width, res.height)));
            return escape_fn(c);
        },
        color_fn
    );
}

template <Complex T, std::invocable<T> F, typename G>
void perturbed_render(std::ostream& os, const resolution& res, const viewport& view, F&& escape_fn, G&& color_fn, multi_complex ref_center) {
    screen_render(os, res,
        [&res, &view, &escape_fn, ref_center](std::size_t x, std::size_t y) {
            T dc = to_complex<T>(view.sample(x, y, res.width, res.height) - ref_center);
            return escape_fn(dc);
        },
        color_fn
    );
}

}   // namespace wacfrac
