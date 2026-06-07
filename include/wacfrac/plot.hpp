#pragma once

#include <wacfrac/color.hpp>
#include <wacfrac/fractal.hpp>
#include <wacfrac/viewport.hpp>
#include <span>
#include <cstddef>

namespace wacfrac
{

struct resolution { std::size_t width, height; };

struct plot {
    resolution   res;
    viewport     view;
    std::size_t  max_iterations; // TODO: calculate dynamically

    auto render_pixel(std::size_t x, std::size_t y) const -> pixel;
    auto render(const std::span<pixel>& buffer) const -> bool;
};

}   // namespace wacfrac
