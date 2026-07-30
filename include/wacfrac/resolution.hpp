#pragma once

#include <cstddef>
#include <sycl/sycl.hpp>

namespace wacfrac
{

    struct Resolution
    {
        std::size_t width, height;
        inline auto area() const { return width * height; }
        inline auto range() const { return sycl::range(width, height); }
    };

} // namespace wacfrac
