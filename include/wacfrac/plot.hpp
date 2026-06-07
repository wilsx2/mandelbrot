#pragma once

#include <wacfrac/color.hpp>
#include <wacfrac/fractal.hpp>
#include <wacfrac/viewport.hpp>
#include <span>
#include <cstddef>

namespace wacfrac
{

struct plot {
    std::size_t  width;
    std::size_t  height;
    viewport     limits;
    unsigned int max_iterations = 1000;

    auto render_pixel(std::size_t x, std::size_t y) const -> pixel;
    auto render(const std::span<pixel>& buffer) const -> bool;
};

}   // namespace wacfrac
