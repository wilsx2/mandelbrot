#pragma once

#include <cstdint>
#include <cstddef>
#include <initializer_list>
#include <tuple>
#include <vector>
#include <complex>

namespace wacfrac
{

struct Pixel { uint8_t r, g, b; };
using Color = std::tuple<float,float,float>;
enum class ColorEncoding { Rgb , Hsv , Hcl };

auto parse_color(std::string_view filename) -> Pixel;

// colorization
// https://www.tomchaplin.xyz/blog/2018-11-02-exploring-the-mandelbrot-set/
// https://linas.org/art-gallery/escape/escape.html
auto colorize_discrete(const std::vector<Pixel>& palette, std::size_t max_n, std::size_t n) -> Pixel;
auto colorize_continuous(const std::vector<Pixel>& palette, std::size_t max_n, std::complex<float> z, std::size_t n) -> Pixel;

// manipulation
auto lerp_pixel(Pixel a, Pixel b, float percentile) -> Pixel;
auto lerp_color(Color a, Color b, float percentile) -> Color;
auto to_pixel(ColorEncoding e, Color c) -> Pixel;
auto rgb_to_pixel(float r, float g, float b) -> Pixel;
auto hsv_to_pixel(float h, float s, float v) -> Pixel;
auto hcl_to_pixel(float l, float c, float h) -> Pixel;

// palette generation
auto generate_palette(std::size_t size, std::initializer_list<Pixel> samples) -> std::vector<Pixel>;
auto generate_palette(std::size_t size, ColorEncoding encoding, std::initializer_list<Color> samples) -> std::vector<Pixel>;

} // namespace wacfrac
