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
    auto generate_probes(std::size_t cols, std::size_t rows) const -> std::vector<std::complex<long double>>;
    auto required_precision() const -> std::size_t;
    auto required_iterations(double modifier = 250.0, double factor = 50.0, double exponent = 1.5) const -> std::size_t;
};

}   // namespace wacfrac
