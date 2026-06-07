#pragma once

#include <boost/multiprecision/mpc.hpp>
#include <boost/multiprecision/gmp.hpp>
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
#include <functional>

namespace mplot
{
using multi_float = boost::multiprecision::mpfr_float;
using multi_complex = boost::multiprecision::mpc_complex;

auto escape_time(multi_complex c, unsigned int max_iterations) -> unsigned int {
    multi_complex z {0.0, 0.0};
    auto n = 0u;

    while (z.real()*z.real() + z.imag()*z.imag() < 4 && n < max_iterations) {
        z = z*z + c;
        ++n;
    }

    return n;
}

auto escape_time_percent(multi_complex c, unsigned int max_iterations) -> float {
    return escape_time(c, max_iterations) / static_cast<float>(max_iterations);
}

struct pixel { uint8_t r, g, b; };

struct axis_limits {
    multi_complex min;
    multi_complex max;


    auto at_zoom(multi_complex focus, multi_float factor) -> axis_limits {
        auto width  = max.real() - min.real();
        auto height = max.imag() - min.imag();

        auto focus_re = (focus.real() - min.real()) / width;
        auto focus_im = (focus.imag() - min.imag()) / height;

        // Scale the view
        auto new_width  = width  / factor;
        auto new_height = height / factor;

        auto new_min = multi_complex(
            focus.real() - focus_re * new_width,
            focus.imag() - focus_im * new_height
        );

        return { new_min, new_min + multi_complex(new_width, new_height) };
    }
    auto sample(size_t x, size_t y, size_t width, size_t height) const -> multi_complex {
        auto re = multi_float(x) / multi_float(width)  * (max.real() - min.real()) + min.real();
        auto im = multi_float(y) / multi_float(height) * (max.imag() - min.imag()) + min.imag();
        return multi_complex(re, im);
    }
};

auto hsv_to_rgb(float h, float s, float v) -> pixel {
    h = std::clamp(h, 0.f, 1.f);
    s = std::clamp(s, 0.f, 1.f);
    v = std::clamp(v, 0.f, 1.f);

    int i = static_cast<int>(std::floor(h * 6.f));
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

auto lch_to_rgb(float l, float c, float h) -> pixel;

struct plot {
    std::size_t width;
    std::size_t height;
    axis_limits limits;
    unsigned int max_iterations = 1000;

    auto render_pixel(std::size_t x, std::size_t y) const -> pixel {
        auto c = limits.sample(x, y, width, height);
        auto percent = escape_time_percent(c, max_iterations);
        if (percent == 1.f)
            return {0, 0, 0};
        auto hue = std::fmod(percent, 0.2f) * 5.0f;
        return hsv_to_rgb(hue, 1.0, 1.0);
    }
    auto render(const std::span<pixel>& buffer) const -> bool {
        std::size_t i = 0;

        auto coords = std::views::cartesian_product(
            std::views::iota(0uz, height),
            std::views::iota(0uz, width)
        );

        for (auto [y,x] : coords) { // Assumes layout 
            if (buffer.size() <= i)
                return false;
            buffer[i++] = render_pixel(x,y);
        }
        return true;
    }
};

auto generate_keyframes();

auto save_plot(std::string_view filename, const plot& p, const std::span<pixel>& buffer) -> bool {
    if (p.width * p.height != buffer.size()) {
        return false;
    }
    if (!p.render(buffer)) {
        return false;
    }

    std::ofstream file(filename.data(), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    auto header = std::format("P6\n{} {}\n255\n", p.width, p.height);
    file.write(header.data(), header.size());
    file.write(
        reinterpret_cast<const char*>(buffer.data()),
        static_cast<std::streamsize>(buffer.size_bytes())
    );
    
    return true;
}

auto save_plot(std::string_view filename, const plot& p) -> bool {
    std::vector<pixel> buffer (p.width * p.height);
    return save_plot(filename, p, buffer);
}

auto save_plots(std::string_view filename, const std::vector<plot>& plots) -> bool {
    std::vector<pixel> buffer;

    for (const auto& [index, p] : std::views::enumerate(plots)) {
        buffer.resize(p.width * p.height);
        
        std::string num_string = std::string((plots.size() + 9) / 10 - (index + 10) / 10, '0') + std::to_string(index);
        std::string final_filename = std::format("{}_{}.ppm", filename, num_string);
        if (!save_plot(final_filename, p, buffer)) {
            return false;
        }
    }

    return true;
}

}   // namespace mplot
