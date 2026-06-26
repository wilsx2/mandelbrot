#pragma once

#include <vector>
#include <wacfrac/types.hpp>
#include <cstddef>

namespace wacfrac
{

struct Viewport {
    MultiComplex center;
    MultiComplex dimensions;

    void precision(std::size_t value);
    auto zoomed(MultiFloat scale) const -> Viewport;
    auto sample(std::size_t x, std::size_t y, std::size_t width, std::size_t height) const -> MultiComplex;
    auto compute_max_dc(MultiComplex c) const -> MultiComplex;
    auto required_precision() const -> std::size_t;
    auto required_iterations(double modifier = 250.0, double factor = 50.0, double exponent = 1.5) const -> std::size_t;
    template<Complex T>
    auto generate_probes(std::size_t cols, std::size_t rows) const -> std::vector<T>;
    template<Complex T>
    auto find_periodic_reference(std::size_t max_n, std::size_t find_period_iter, std::size_t find_nucleus_iter) const -> std::pair<MultiComplex, std::vector<T>>;
};
auto required_precision(MultiFloat zoom_factor) -> std::size_t;
auto required_iterations(MultiFloat zoom_factor, double modifier = 250.0, double factor = 50.0, double exponent = 1.5) -> std::size_t;

}   // namespace wacfrac
