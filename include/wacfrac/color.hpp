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

auto colorize_discrete(const std::vector<Pixel>& palette, std::size_t max_n, std::size_t n) -> Pixel;
auto colorize_continuous(const std::vector<Pixel>& palette, std::size_t max_n, std::complex<float> z, std::size_t n) -> Pixel;

} // namespace wacfrac
