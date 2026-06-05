#pragma once

#include <complex>
#include <cmath>
#include <concepts>
#include <ranges>
#include <fstream>
#include <format>
#include <span>
#include <mdspan>
#include <vector>
#include <cstdint>

namespace mplot
{

constexpr std::complex<double> Z_0 = {0.0, 0.0};

auto escape_time(std::complex<double> c, unsigned int max_iterations) -> unsigned int {
    auto z_n = Z_0;
    auto n = 0u;

    while (std::norm(z_n) <= 2*2 && n < max_iterations) {
        z_n = z_n * z_n + c;
        ++n;
    }

    return n;
}

auto escape_time_percent(std::complex<double> c, unsigned int max_iterations) -> float {
    return escape_time(c, max_iterations) / static_cast<float>(max_iterations);
}

struct pixel { uint8_t r, g, b; };

struct axis_limits {
    std::complex<double> min;
    std::complex<double> max;

    void zoom(std::complex<double> focus, double factor) {
        min = (1/factor) * min + (1 - 1/factor) * focus;
        max = (1/factor) * max + (1 - 1/factor) * focus;
    }
    auto sample(size_t x, size_t y, size_t width, size_t height) const -> std::complex<double> {
        return {
            x / static_cast<double>(width)  * (std::real(max) - std::real(min)) + std::real(min),
            y / static_cast<double>(height) * (std::imag(max) - std::imag(min)) + std::imag(min)
        };
    }
};

auto hsv_to_rgb(float h, float s, float v) -> pixel {
    h = std::clamp(h, 0.f, 1.f);
    s = std::clamp(s, 0.f, 1.f);
    v = std::clamp(v, 0.f, 1.f);

    int i = static_cast<int>(std::floor(h * 6.f));
    float f = h - i;
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
        case 5: r = f; g = p; b = q; break;
        default: r = g = b = 0.f;    break;
    }

    return {
        static_cast<uint8_t>(std::round(r * 255.f)),
        static_cast<uint8_t>(std::round(g * 255.f)),
        static_cast<uint8_t>(std::round(b * 255.f))
    };
}

auto lch_to_rgb(float l, float c, float h) -> pixel;

auto coordinate_view(size_t rows, size_t cols) {
    return std::views::cartesian_product(
        std::views::iota(0uz, rows),
        std::views::iota(0uz, cols)
    );
}

template <std::invocable<std::complex<double>> F>
auto save_to_ppm(std::string_view filename, std::size_t width, std::size_t height,
                 const axis_limits& lim, F&& colorize) -> bool {
    std::ofstream file(filename.data(), std::ios::binary);

    if (!file.is_open())
        return false;

    auto header = std::format("P6\n{} {}\n255\n", width, height);
    file.write(header.data(), header.size());

    for (auto [y,x] : coordinate_view(height, width)) {
        auto color = colorize(lim.sample(x, y, width, height));
        file.write(
            reinterpret_cast<const char*>(&color),
            static_cast<std::streamsize>(sizeof(color))
        );
    }
    
    return true;
}

}   // namespace mplot
