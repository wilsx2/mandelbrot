#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <complex>
#include <ranges>
#include <functional>

namespace wacfrac
{

struct rgb { uint8_t r, g, b; };

// colorization
enum class colorization_type { discrete , continuous };
enum class colorization_method { normal , looped };
struct colorization_algorithm {
    colorization_type type;
    colorization_method method;
};
auto colorize(colorization_algorithm ca, const std::vector<rgb>& palette, std::size_t max_n, std::complex<double> z, std::size_t n) -> rgb;
auto colorize_discrete(colorization_method method, const std::vector<rgb>& palette, std::size_t max_n, std::size_t n) -> rgb;
auto colorize_continuous(colorization_method method, const std::vector<rgb>& palette, std::size_t max_n, std::complex<double> z, std::size_t n) -> rgb;
auto palette_lookup_normal(const std::vector<rgb>& palette, std::size_t max_n, std::size_t n) -> rgb;
auto palette_lookup_looped(const std::vector<rgb>& palette, std::size_t max_n, std::size_t n) -> rgb;

// manipulation
auto lerp_color(rgb a, rgb b, float percentile) -> rgb;
auto hsv_to_rgb(float h, float s, float v) -> rgb;
auto hcl_to_rgb(float l, float c, float h) -> rgb;

// palette generation
template<std::invocable<float> F>
auto color_generator(F&& func, float increment, float initial) -> std::function<rgb()> {
    return [=]() {
        static float value = initial - increment;
        return func(value = std::fmod(value + increment, 1.f));
    };
}

template<std::invocable<float> F>
auto generate_palette(F&& func, std::size_t samples, float initial = 0.f, float range = 1.f) -> std::vector<rgb> {
    std::vector<rgb> palette (samples);
    palette.front() = rgb(0, 0, 0);
    auto generator {color_generator(func, range/static_cast<float>(samples-2), initial)};
    std::generate(palette.begin() + 1, palette.end(), generator);
    return palette;
}

}   // namespace wacfrac
