#pragma once

#include <cstdint>
#include <cstddef>
#include <initializer_list>
#include <vector>

#if defined(__CUDACC__)
    #include <cuda/std/complex>
    #include <cuda/std/span>
    #define STD cuda::std
#else 
    #include <complex>
    #include <span>
    #define STD std
#endif

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

#if defined(__CUDACC__)
__host__ __device__
#endif 
inline auto colorize_discrete(std::size_t n, std::size_t max_n, STD::span<const Pixel> palette) -> Pixel {
    if (n == max_n)
        return palette.back();
    return palette[n % palette.size()];
}

#if defined(__CUDACC__)
__host__ __device__
#endif 
inline auto colorize_continuous(STD::complex<float> z, std::size_t n, std::size_t max_n, STD::span<const Pixel> palette) -> Pixel
{
    if (n == max_n)
        return palette.back();
    auto cont_n {n - STD::log(std::log(STD::abs(z))) / STD::log(2.0)};
    auto n1     {static_cast<std::size_t>(STD::floor(cont_n))};
    auto n2     {static_cast<std::size_t>(STD::ceil(cont_n))};
    auto color1 {palette[n1 % palette.size()]};
    auto color2 {palette[n2 % palette.size()]};
    auto progress {STD::fmod(cont_n, 1.0)};
    return {
        static_cast<uint8_t>(color2.r * progress + color1.r * (1.0 - progress)),
        static_cast<uint8_t>(color2.g * progress + color1.g * (1.0 - progress)),
        static_cast<uint8_t>(color2.b * progress + color1.b * (1.0 - progress))
    };
}

} // namespace wacfrac

#undef STD
