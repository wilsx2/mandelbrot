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
#include <functional>

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

    auto at_zoom(std::complex<double> focus, double factor) -> axis_limits {
        return {
            {(1/factor) * min + (1 - 1/factor) * focus},
            {(1/factor) * max + (1 - 1/factor) * focus}
        };
    }
    auto sample(size_t x, size_t y, size_t width, size_t height) const -> std::complex<double> {
        return {
            x / static_cast<double>(width)  * (std::real(max) - std::real(min)) + std::real(min),
            y / static_cast<double>(height) * (std::imag(max) - std::imag(min)) + std::imag(min)
        };
    }
};

struct job {
    std::size_t width;
    std::size_t height;
    axis_limits limits;
    std::function<pixel(std::complex<double>)> coloring_algorithm;

    auto render_pixel(std::size_t x, std::size_t y) const -> pixel {
        return coloring_algorithm(limits.sample(x, y, width, height));
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

auto generate_keyframes();

auto save_job(std::string_view filename, const job& j, const std::span<pixel>& buffer) -> bool {
    if (j.width * j.height != buffer.size()) {
        return false;
    }
    if (!j.render(buffer)) {
        return false;
    }

    std::ofstream file(filename.data(), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    auto header = std::format("P6\n{} {}\n255\n", j.width, j.height);
    file.write(header.data(), header.size());
    file.write(
        reinterpret_cast<const char*>(buffer.data()),
        static_cast<std::streamsize>(buffer.size_bytes())
    );
    
    return true;
}

auto save_job(std::string_view filename, const job& j) -> bool {
    std::vector<pixel> buffer (j.width * j.height);
    return save_job(filename, j, buffer);
}

auto save_jobs(std::string_view filename, const std::vector<job>& jobs) -> bool {
    std::vector<pixel> buffer;

    for (const auto& [index, j] : std::views::enumerate(jobs)) {
        buffer.resize(j.width * j.height);
        if (!j.render(buffer)) {
            return false;
        }
        
        std::string num_string = std::string((jobs.size() + 9) / 10 - (index + 10) / 10, '0') + std::to_string(index);
        std::string final_filename = std::format("{}_{}.ppm", filename, num_string);
        if (!save_job(final_filename, j, buffer)) {
            return false;
        }
    }

    return true;
}

}   // namespace mplot
