#pragma once

#include <wacfrac/color.hpp>
#include <wacfrac/fractal.hpp>
#include <wacfrac/viewport.hpp>
#include <span>
#include <ranges>
#include <cstddef>

namespace wacfrac
{

struct resolution {
    std::size_t width, height;
    inline auto area() const {
        return width * height;
    }
    inline auto coordinates() const {
        return std::views::cartesian_product(
            std::views::iota(0uz, height),
            std::views::iota(0uz, width)
        );
    }
};

struct plot {
    resolution   res;
    viewport     view;
    std::size_t  max_iterations; // TODO: calculate dynamically
};

auto render_directly      (const plot& p, const std::span<pixel>& buffer) -> bool;
auto render_perturbed     (const plot& p, const std::span<pixel>& buffer) -> bool;

}   // namespace wacfrac
