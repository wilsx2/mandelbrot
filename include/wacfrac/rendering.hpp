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

struct direct_eta { };
struct perturbed_eta { };
struct approximate_eta {
    std::size_t num_coefficients, probe_cols, probe_rows;
    double tolerance;
};
using escape_time_algorithm = std::variant<direct_eta, perturbed_eta, approximate_eta>;

struct render_config {
    resolution  res;
    viewport    view;
    std::size_t max_iterations;
    escape_time_algorithm eta;
};

auto render(const render_config& conf, const std::span<pixel>& buffer) -> bool;

}   // namespace wacfrac
