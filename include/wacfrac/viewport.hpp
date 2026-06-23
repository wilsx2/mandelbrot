#pragma once

#include <vector>
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
    auto compute_max_dc(multi_complex c) const -> multi_complex;
    auto required_precision() const -> std::size_t;
    auto required_iterations(double modifier = 250.0, double factor = 50.0, double exponent = 1.5) const -> std::size_t;
    template<Complex T>
    auto generate_probes(std::size_t cols, std::size_t rows) const -> std::vector<T>;
    template<Complex T>
    auto find_periodic_reference(std::size_t max_n, std::size_t find_period_iter, std::size_t find_nucleus_iter) const -> std::pair<multi_complex, std::vector<T>>;
};
auto required_precision(multi_float zoom_factor) -> std::size_t;
auto required_iterations(multi_float zoom_factor, double modifier = 250.0, double factor = 50.0, double exponent = 1.5) -> std::size_t;

}   // namespace wacfrac
