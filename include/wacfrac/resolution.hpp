#pragma once

#include <sycl/sycl.hpp>
#include <cstddef>

namespace wacfrac {

struct Resolution {
    std::size_t width, height;
    inline auto area() const {
        return width * height;
    }
    inline auto range() const {
        return sycl::range(width, height);
    }
};

} // namespace wacfrac
