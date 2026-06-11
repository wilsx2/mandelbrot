#pragma once

#include <wacfrac/types.hpp>
#include <cstddef>

namespace wacfrac
{

struct viewport {
    multi_complex center;
    multi_complex dimensions;

    void precision(std::size_t value);
    auto zoomed(multi_float factor) const -> viewport;
    auto sample(std::size_t x, std::size_t y, std::size_t width, std::size_t height) const -> multi_complex;
};

}   // namespace wacfrac
