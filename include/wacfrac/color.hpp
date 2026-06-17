#pragma once

#include <cstdint>
#include <cstddef>
#include <initializer_list>
#include <tuple>
#include <vector>
#include <complex>
#include <ranges>
#include <functional>

namespace wacfrac
{

struct pixel { uint8_t r, g, b; };
using color = std::tuple<float,float,float>;
enum class color_encoding { rgb , hsv , hcl };

// colorization
enum class colorization_type { discrete , continuous };
enum class colorization_method { normal , looped };
struct colorization_algorithm {
    colorization_type type;
    colorization_method method;
};
auto colorize(colorization_algorithm ca, const std::vector<pixel>& palette, std::size_t max_n, std::complex<double> z, std::size_t n) -> pixel;
auto colorize_discrete(colorization_method method, const std::vector<pixel>& palette, std::size_t max_n, std::size_t n) -> pixel;
auto colorize_continuous(colorization_method method, const std::vector<pixel>& palette, std::size_t max_n, std::complex<double> z, std::size_t n) -> pixel;
auto palette_lookup_normal(const std::vector<pixel>& palette, std::size_t max_n, std::size_t n) -> pixel;
auto palette_lookup_looped(const std::vector<pixel>& palette, std::size_t n) -> pixel;

// manipulation
auto lerp_pixel(pixel a, pixel b, float percentile) -> pixel;
auto lerp_color(color a, color b, float percentile) -> color;
auto to_pixel(color_encoding e, color c) -> pixel;
auto rgb_to_pixel(float r, float g, float b) -> pixel;
auto hsv_to_pixel(float h, float s, float v) -> pixel;
auto hcl_to_pixel(float l, float c, float h) -> pixel;

// palette generation
auto generate_palette(std::size_t size, std::initializer_list<pixel> samples) -> std::vector<pixel>;
auto generate_palette(std::size_t size, color_encoding encoding, std::initializer_list<color> samples) -> std::vector<pixel>;

} // namespace wacfrac
