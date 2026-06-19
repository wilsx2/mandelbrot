#pragma once

#include <wacfrac/color.hpp>
#include <wacfrac/orbit.hpp>
#include <wacfrac/viewport.hpp>
#include <wacfrac/resolution.hpp>
#include <span>
#include <cstddef>

namespace wacfrac
{

struct direct_eta { };
struct perturbed_eta { };
struct approximate_eta {
    std::size_t num_coefficients, probe_cols, probe_rows;
    double tolerance;
};
struct bla_eta {
    double epsilon;
    std::size_t first_level;
};
using escape_time_algorithm = std::variant<direct_eta, perturbed_eta, approximate_eta, bla_eta>;

struct render_config {
    resolution  res;
    viewport    view;
    std::size_t max_iterations;
    std::vector<pixel> palette;
    escape_time_algorithm eta;
    colorization_algorithm ca;
};

auto render(const render_config& conf, const std::span<pixel>& buffer) -> bool;

}   // namespace wacfrac
