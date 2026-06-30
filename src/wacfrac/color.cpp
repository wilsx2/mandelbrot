#include <concepts>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <wacfrac/color.hpp>
#include <wacfrac/log.hpp>
#include <wacfrac/orbit.hpp>

#include <algorithm>
#include <ranges>
#include <cmath>

namespace wacfrac
{

auto parse_color(std::string_view string) -> Pixel {
    if (string.size() < 7 || string[0] != '#') {
        logging::error( "Error parsing string as a color; size or prefix: {}", string);
        return {255,0,255};
    }

    std::stringstream ss {string.data() + 1};

    unsigned int hex_color;
    if (!(ss >> std::hex >> hex_color)) {
        logging::error( "Error parsing string as a color; to hex: {}", string);
        return {255,0,255};
    }

    Pixel color {
        static_cast<uint8_t>((hex_color >> 16) & 0xFF),
        static_cast<uint8_t>((hex_color >> 8) & 0xFF),
        static_cast<uint8_t>(hex_color & 0xFF)
    };
    logging::debug( "Parsed color '{}' as rgb({},{},{})", color.r, color.g, color.b);
    return color;
}

auto colorize_continuous(const std::vector<Pixel>& palette, std::size_t max_n, std::complex<float> z, std::size_t n) -> Pixel {
    if (n == max_n)
        return palette.back();
    auto cont_n {n - std::log(std::log(std::abs(z))) / std::log(2.0)};
    auto n1     {static_cast<std::size_t>(std::floor(cont_n))};
    auto n2     {static_cast<std::size_t>(std::ceil(cont_n))};
    auto color1 {colorize_discrete(palette, max_n, n1)};
    auto color2 {colorize_discrete(palette, max_n, n2)};
    return lerp_pixel(color1, color2, std::fmod(cont_n, 1.0));
}

auto colorize_discrete(const std::vector<Pixel>& palette, std::size_t max_n, std::size_t n) -> Pixel {
    if (n == max_n)
        return palette.back();
    return palette.at(n % palette.size());
}

auto lerp_pixel(Pixel a, Pixel b, float percentile) -> Pixel {
    return Pixel(
        std::lerp(a.r, b.r, percentile),
        std::lerp(a.g, b.g, percentile),
        std::lerp(a.b, b.b, percentile)
    );
}

auto lerp_color(Color a, Color b, float percentile) -> Color {
    return Color(
        std::lerp(std::get<0>(a), std::get<0>(b), percentile),
        std::lerp(std::get<1>(a), std::get<1>(b), percentile),
        std::lerp(std::get<2>(a), std::get<2>(b), percentile)
    );
}

auto to_pixel(ColorEncoding e, Color c) -> Pixel {
    switch (e) {
        case ColorEncoding::Rgb: return std::apply(rgb_to_pixel, c);
        case ColorEncoding::Hsv: return std::apply(hsv_to_pixel, c);
        case ColorEncoding::Hcl: return std::apply(hcl_to_pixel, c);
    }
  return {255, 0, 255};
}

auto rgb_to_pixel(float r, float g, float b) -> Pixel {
    r = std::clamp(r, 0.f, 1.f);
    g = std::clamp(g, 0.f, 1.f);
    b = std::clamp(b, 0.f, 1.f);
    return Pixel(r*255, g*255, b*255);
}

auto hsv_to_pixel(float h, float s, float v) -> Pixel {
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

auto hcl_to_pixel(float h, float c, float l) -> Pixel {
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

template <typename T, std::invocable<T, T, float> F>
static auto generate_palette(std::size_t size, std::initializer_list<T> samples, F&& generate) -> std::vector<Pixel> {
    if (size == 0 || samples.size() == 0) return {};
      
      std::vector<Pixel> palette(size);
      const auto samples_view = std::views::all(samples);
      const std::size_t num_segments = samples.size() - 1;

      for (auto&& [i, col] : std::views::enumerate(palette)) {
          float global_t = (size > 1) ? static_cast<float>(i) / (size - 1) : 0.0f;

          float segment_scaled = global_t * num_segments;
          std::size_t segment_idx = std::min(static_cast<std::size_t>(segment_scaled), num_segments - 1);

          float progress = segment_scaled - segment_idx;

          auto curr = samples_view[segment_idx];
          auto next = samples_view[std::min(segment_idx + 1, samples.size() - 1)];

          col = generate(curr, next, progress);
      }

      return palette;
}

auto generate_palette(std::size_t size, std::initializer_list<Pixel> samples) -> std::vector<Pixel> {
    return generate_palette(size, samples, lerp_pixel);
}

auto generate_palette(std::size_t size, ColorEncoding encoding, std::initializer_list<Color> samples) -> std::vector<Pixel> {
    return generate_palette(size, samples, [encoding](Color a, Color b, float progress){
        return to_pixel(encoding, lerp_color(a, b, progress));
    });
}

}   // namespace wacfrac
