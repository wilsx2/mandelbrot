#pragma once

#include <wacfrac/types.hpp>
#include <cstddef>

namespace wacfrac
{

struct viewport {
    multi_complex center;
    multi_complex dimensions;

    void precision(std::size_t value);
    auto zoomed(multi_float scale) const -> viewport;
    auto sample(std::size_t x, std::size_t y, std::size_t width, std::size_t height) const -> multi_complex;
    auto generate_probes(std::size_t cols, std::size_t rows) const -> std::vector<std::complex<double>>;
};

auto approximate_required_precision(multi_float scale) -> unsigned int;
auto approximate_required_iterations(multi_float scale, double modifier = 250.0, double factor = 50.0, double exponent = 1.5) -> unsigned int;

}   // namespace wacfrac
