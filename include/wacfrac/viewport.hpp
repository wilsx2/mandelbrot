#pragma once

#include <wacfrac/types.hpp>
#include <cstddef>

namespace wacfrac
{

struct viewport {
    multi_complex min;
    multi_complex max;

    void precision(std::size_t value);
    auto at_zoom(multi_complex focus, multi_float factor) -> viewport;
    auto sample(std::size_t x, std::size_t y, std::size_t width, std::size_t height) const -> multi_complex;
};

}   // namespace wacfrac
