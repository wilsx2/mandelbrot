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

inline const std::vector<Pixel> ULTRA {
    // https://stackoverflow.com/questions/16500656/which-color-gradient-is-used-to-color-mandelbrot-in-wikipedia
    {   9,   1,  47 },
    {   4,   4,  73 },
    {   0,   7, 100 },
    {  12,  44, 138 },
    {  24,  82, 177 },
    {  57, 125, 209 },
    { 134, 181, 229 },
    { 211, 236, 248 },
    { 241, 233, 191 },
    { 248, 201,  95 },
    { 255, 170,   0 },
    { 204, 128,   0 },
    { 153,  87,   0 },
    { 106,  52,   3 },
    {  66,  30,  15 },
    {  25,   7,  26 },
};

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
