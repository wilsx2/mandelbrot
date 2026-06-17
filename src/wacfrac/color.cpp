#include <vector>
#include <wacfrac/color.hpp>
#include <wacfrac/orbit.hpp>

#include <algorithm>
#include <cmath>

namespace wacfrac
{

// https://www.tomchaplin.xyz/blog/2018-11-02-exploring-the-mandelbrot-set/
// https://linas.org/art-gallery/escape/escape.html
auto colorize(colorization_algorithm ca, const std::vector<pixel>& palette, std::size_t max_n, std::complex<double> z, std::size_t n) -> pixel {
    if (n == max_n)
        return {0,0,0};
    switch (ca.type) {
        case colorization_type::discrete:   return colorize_discrete(ca.method, palette, max_n, n); break;
        case colorization_type::continuous: return colorize_continuous(ca.method, palette, max_n, z, n); break;
    }
    return {255, 0, 255};
}
auto colorize_discrete(colorization_method method, const std::vector<pixel>& palette, std::size_t max_n, std::size_t n) -> pixel {
    switch (method) {
        case colorization_method::normal: return palette_lookup_normal(palette, max_n, n);
        case colorization_method::looped: return palette_lookup_looped(palette, n);
    }
    return {255, 0, 255};
}
auto colorize_continuous(colorization_method method, const std::vector<pixel>& palette, std::size_t max_n, std::complex<double> z, std::size_t n) -> pixel {
    auto cont_n {n - std::log(std::log(magnitude(z))) / std::log(2.0)};
    auto n1     {static_cast<std::size_t>(std::floor(cont_n))};
    auto n2     {static_cast<std::size_t>(std::ceil(cont_n))};
    auto color1 {colorize_discrete(method, palette, max_n, n1)};
    auto color2 {colorize_discrete(method, palette, max_n, n2)};
    return lerp_pixel(color1, color2, std::fmod(cont_n, 1.0));
}
auto palette_lookup_normal(const std::vector<pixel>& palette, std::size_t max_n, std::size_t n) -> pixel {
    return palette.at(std::floor((n / static_cast<float>(max_n)) * static_cast<float>(palette.size() - 1)));
}
auto palette_lookup_looped(const std::vector<pixel>& palette, std::size_t n) -> pixel {
    return palette.at(n % palette.size());
}

auto lerp_pixel(pixel a, pixel b, float percentile) -> pixel {
    return pixel(
        a.r * (1.f - percentile) + b.r * (percentile),
        a.g * (1.f - percentile) + b.g * (percentile),
        a.b * (1.f - percentile) + b.b * (percentile)
    );
}

auto lerp_color(color a, color b, float percentile) -> color {
    return color(
        std::get<0>(a) * (1.f - percentile) + std::get<0>(b) * (percentile),
        std::get<1>(a) * (1.f - percentile) + std::get<1>(b) * (percentile),
        std::get<2>(a) * (1.f - percentile) + std::get<2>(b) * (percentile)
    );
}

auto to_pixel(color_encoding e, color c) -> pixel {
    switch (e) {
        case color_encoding::rgb: return std::apply(rgb_to_pixel, c);
        case color_encoding::hsv: return std::apply(hsv_to_pixel, c);
        case color_encoding::hcl: return std::apply(hcl_to_pixel, c);
    }
  return {255, 0, 255};
}

auto rgb_to_pixel(float r, float g, float b) -> pixel {
    r = std::clamp(r, 0.f, 1.f);
    g = std::clamp(g, 0.f, 1.f);
    b = std::clamp(b, 0.f, 1.f);
    return pixel(r*255, g*255, b*255);
}

auto hsv_to_pixel(float h, float s, float v) -> pixel {
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

auto hcl_to_pixel(float h, float c, float l) -> pixel {
    h = std::clamp(h, 0.f, 1.f);
    c = std::clamp(c, 0.f, 1.f);
    l = std::clamp(l, 0.f, 1.f);

    float v = l;

    float s = (v > 0.f) ? std::clamp(c / v, 0.f, 1.f) : 0.f;

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

auto generate_palette(std::size_t size, std::initializer_list<pixel> samples) -> std::vector<pixel> {
    if (size == 0 || samples.size() == 0) return {};
    
    std::vector<pixel> palette(size);
    const auto samples_view = std::views::all(samples);
    const std::size_t num_segments = samples.size() - 1;

    for (auto&& [i, col] : std::views::enumerate(palette)) {
        float global_t = (size > 1) ? static_cast<float>(i) / (size - 1) : 0.0f;

        float segment_scaled = global_t * num_segments;
        std::size_t segment_idx = std::min(static_cast<std::size_t>(segment_scaled), num_segments - 1);

        float progress = segment_scaled - segment_idx;

        auto curr = samples_view[segment_idx];
        auto next = samples_view[std::min(segment_idx + 1, samples.size() - 1)];

        col = lerp_pixel(curr, next, progress);
    }

    return palette;
}

auto generate_palette(std::size_t size, color_encoding encoding, std::initializer_list<color> samples) -> std::vector<pixel> {
    if (size == 0 || samples.size() == 0) return {};
    
    std::vector<pixel> palette(size);
    const auto samples_view = std::views::all(samples);
    const std::size_t num_segments = samples.size() - 1;

    for (auto&& [i, col] : std::views::enumerate(palette)) {
        float global_t = (size > 1) ? static_cast<float>(i) / (size - 1) : 0.0f;

        float segment_scaled = global_t * num_segments;
        std::size_t segment_idx = std::min(static_cast<std::size_t>(segment_scaled), num_segments - 1);

        float progress = segment_scaled - segment_idx;

        auto curr = samples_view[segment_idx];
        auto next = samples_view[std::min(segment_idx + 1, samples.size() - 1)];

        col = to_pixel(encoding, lerp_color(curr, next, progress));
    }

    return palette;
}

}   // namespace wacfrac
