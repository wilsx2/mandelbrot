#pragma once

#include <sycl/sycl.hpp>
#include <cstddef>
#include <ranges>

namespace wacfrac {

struct Resolution {
    std::size_t width, height;
    inline auto area() const {
        return width * height;
    }
#if __cpp_lib_ranges_cartesian_product >= 202207L
    inline auto coordinates() const {
        return std::views::cartesian_product(
            std::views::iota(0uz, height),
            std::views::iota(0uz, width)
        );
    }
#endif
    inline auto range() const {
        return sycl::range(width, height);
    }
};

} // namespace wacfrac
