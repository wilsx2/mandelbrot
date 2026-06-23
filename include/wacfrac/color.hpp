#pragma once

#include <cstdint>
#include <cstddef>
#include <initializer_list>
#include <tuple>
#include <vector>
#include <complex>

namespace wacfrac
{

struct pixel { uint8_t r, g, b; };
using color = std::tuple<float,float,float>;
enum class color_encoding { rgb , hsv , hcl };

// colorization
// https://www.tomchaplin.xyz/blog/2018-11-02-exploring-the-mandelbrot-set/
// https://linas.org/art-gallery/escape/escape.html
template <std::invocable<std::size_t> F>
auto colorize_continuous(F&& lookup, std::complex<float> z, std::size_t n) -> pixel {
    auto cont_n {n - std::log(std::log(std::abs(z))) / std::log(2.0)};
    auto n1     {static_cast<std::size_t>(std::floor(cont_n))};
    auto n2     {static_cast<std::size_t>(std::ceil(cont_n))};
    auto color1 {lookup(n1)};
    auto color2 {lookup(n2)};
    return lerp_pixel(color1, color2, std::fmod(cont_n, 1.0));
}
auto colorize_normal(const std::vector<pixel>& palette, std::size_t max_n, std::size_t n) -> pixel;
auto colorize_looped(const std::vector<pixel>& palette, std::size_t n) -> pixel;

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
