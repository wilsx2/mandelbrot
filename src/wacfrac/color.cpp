#include <wacfrac/color.hpp>

#include <algorithm>
#include <cmath>

namespace wacfrac
{

auto colorize(colorization_algorithm ca, const std::vector<rgb>& palette, std::size_t max_n, std::size_t n) -> rgb {
    switch (ca) {
        case wacfrac::colorization_algorithm::normal: return colorize_normal(palette, max_n, n);
        case wacfrac::colorization_algorithm::looped: return colorize_looped(palette, max_n, n);
    }
    return {255, 0, 255};
}

auto colorize_normal(const std::vector<rgb>& palette, std::size_t max_n, std::size_t n) -> rgb {
    return palette.at((max_n - n) / static_cast<float>(max_n) * (palette.size() - 2));
}
auto colorize_looped(const std::vector<rgb>& palette, std::size_t max_n, std::size_t n) -> rgb {
    return palette.at((max_n - n) % palette.size());
}

auto hsv_rainbow_generator(float increment, float initial_hue) -> std::function<rgb()> {
    return [increment, initial_hue]() {
        static float hue = initial_hue;
        return hsv_to_rgb(hue = std::fmod(hue + increment, 1.f), 1.f, 1.f);
    };
}
auto generate_hsv_rainbow_palette(std::size_t samples, float initial_hue, float range) -> std::vector<rgb> {
    std::vector<rgb> palette (samples);
    palette.front() = rgb(0, 0, 0);
    palette.back()  = rgb(0, 0, 0);
    auto generator {hsv_rainbow_generator(range/static_cast<float>(samples-2), initial_hue)};
    std::ranges::generate(palette.begin() + 1, palette.end() - 1, generator);
    return palette;
}

auto hsv_to_rgb(float h, float s, float v) -> rgb {
    h = std::clamp(h, 0.f, 1.f);
    s = std::clamp(s, 0.f, 1.f);
    v = std::clamp(v, 0.f, 1.f);

    int   i = static_cast<int>(std::floor(h * 6.f));
    float f = h * 6.f - i;
    float p = v * (1.f - s);
    float q = v * (1.f - s * f);
    float t = v * (1.f - s * (1.f - f));

    float r, g, b;
    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
        default: r = g = b = 0.f;    break;
    }

    return {
        static_cast<uint8_t>(std::round(r * 255.f)),
        static_cast<uint8_t>(std::round(g * 255.f)),
        static_cast<uint8_t>(std::round(b * 255.f))
    };
}

}   // namespace wacfrac
