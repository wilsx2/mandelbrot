#include <wacfrac/plot.hpp>

#include <cmath>
#include <ranges>

namespace wacfrac
{

auto plot::render_pixel(std::size_t x, std::size_t y) const -> pixel {
    auto c       = limits.sample(x, y, width, height);
    auto percent = escape_time_percent(c, max_iterations);
    if (percent == 1.f)
        return {0, 0, 0};
    auto hue = std::fmod(percent, 0.2f) * 5.0f;
    return hsv_to_rgb(hue, 1.0f, 1.0f);
}

auto plot::render(const std::span<pixel>& buffer) const -> bool {
    std::size_t i = 0;

    auto coords = std::views::cartesian_product(
        std::views::iota(0uz, height),
        std::views::iota(0uz, width)
    );

    for (auto [y, x] : coords) {    // row-major: y is outer, x is inner
        if (buffer.size() <= i)
            return false;
        buffer[i++] = render_pixel(x, y);
    }
    return true;
}

}   // namespace wacfrac
