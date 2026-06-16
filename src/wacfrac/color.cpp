#include <wacfrac/color.hpp>
#include <wacfrac/fractal.hpp>

#include <algorithm>
#include <cmath>

namespace wacfrac
{

// https://www.tomchaplin.xyz/blog/2018-11-02-exploring-the-mandelbrot-set/
// https://linas.org/art-gallery/escape/escape.html
auto colorize(colorization_algorithm ca, const std::vector<rgb>& palette, std::size_t max_n, std::complex<double> z, std::size_t n) -> rgb {
    switch (ca.type) {
        case colorization_type::discrete:   return colorize_discrete(ca.method, palette, max_n, n); break;
        case colorization_type::continuous: return colorize_continuous(ca.method, palette, max_n, z, n); break;
    }
    return {255, 0, 255};
}
auto colorize_discrete(colorization_method method, const std::vector<rgb>& palette, std::size_t max_n, std::size_t n) -> rgb {
    switch (method) {
        case colorization_method::normal: return palette_lookup_normal(palette, max_n, n);
        case colorization_method::looped: return palette_lookup_looped(palette, max_n, n);
    }
    return {255, 0, 255};
}
auto colorize_continuous(colorization_method method, const std::vector<rgb>& palette, std::size_t max_n, std::complex<double> z, std::size_t n) -> rgb {
    auto cont_n {n - (std::log(std::log(magnitude(z))))/std::log(2.0)};
    auto n1     {static_cast<std::size_t>(std::floor(cont_n))};
    auto n2     {static_cast<std::size_t>(std::ceil(cont_n))};
    auto color1 {colorize_discrete(method, palette, max_n, n1)};
    auto color2 {colorize_discrete(method, palette, max_n, n2)};
    return lerp_color(color1, color2, std::fmod(cont_n, 1.0));
}
auto palette_lookup_normal(const std::vector<rgb>& palette, std::size_t max_n, std::size_t n) -> rgb {
    return palette.at((max_n - n) / static_cast<float>(max_n) * (palette.size() - 2));
}
auto palette_lookup_looped(const std::vector<rgb>& palette, std::size_t max_n, std::size_t n) -> rgb {
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

auto lerp_color(rgb a, rgb b, float percentile) -> rgb {
    return rgb(
        a.r * (1.f - percentile) + b.r * (percentile),
        a.g * (1.f - percentile) + b.g * (percentile),
        a.b * (1.f - percentile) + b.b * (percentile)
    );
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
