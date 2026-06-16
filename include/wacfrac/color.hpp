#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <functional>

namespace wacfrac
{

struct rgb { uint8_t r, g, b; };

// colorization
enum class colorization_algorithm { normal , looped };
auto colorize(colorization_algorithm ca, const std::vector<rgb>& palette, std::size_t max_n, std::size_t n) -> rgb;
auto colorize_normal(const std::vector<rgb>& palette, std::size_t max_n, std::size_t n) -> rgb;
auto colorize_looped(const std::vector<rgb>& palette, std::size_t max_n, std::size_t n) -> rgb;

// palette generation
auto hsv_rainbow_generator(float increment, float initial_hue) -> std::function<rgb()>;
auto generate_hsv_rainbow_palette(std::size_t samples, float initial_hue = 1.f, float range = 1.f) -> std::vector<rgb>;

// conversion
auto hsv_to_rgb(float h, float s, float v) -> rgb;
auto lch_to_rgb(float l, float c, float h) -> rgb; // TODO: Implement

}   // namespace wacfrac
