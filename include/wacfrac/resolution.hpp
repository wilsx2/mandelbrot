#pragma once

#include <cstddef>
#include <ranges>

namespace wacfrac {

struct Resolution {
    std::size_t width, height;
    inline auto area() const {
        return width * height;
    }
    inline auto coordinates() const {
        return std::views::cartesian_product(
            std::views::iota(0uz, height),
            std::views::iota(0uz, width)
        );
    }
};

} // namespace wacfrac
