#pragma once

#include "wacfrac/color.hpp"
#include "wacfrac/resolution.hpp"
#include "wacfrac/types.hpp"
#include <algorithm>
#include <concepts>
#include <filesystem>
#include <ranges>
#include <wacfrac/rendering.hpp>
#include <span>
#include <string>
#include <string_view>

namespace wacfrac
{

void write_ppm(std::string_view filename, Resolution res, std::span<const Pixel> pixels);

template <std::invocable<std::span<Pixel>, MultiFloat> F>
void write_zoom_frames(std::filesystem::path directory, Resolution res, MultiFloat initial, MultiFloat target, MultiFloat zoom_per_second, float frames_per_second, F&& render_at_scale) {
    if (!std::filesystem::create_directories(directory))
        return;
    
    using boost::multiprecision::log;
    using boost::multiprecision::exp;
    using boost::multiprecision::ceil;
    auto zoom_per_frame {exp(log(zoom_per_second) / frames_per_second)};
    auto frames {static_cast<std::size_t>(ceil(log(target / initial) / log(zoom_per_frame)))};
    auto suffix_width = frames > 0 ? std::to_string(frames - 1).length() : 1;
    std::vector<Pixel> pixels (res.area());
    for (auto frame : std::views::iota(0uz, frames)) {
        render_at_scale(pixels, initial);
        write_ppm(std::format("{}/frame_{:0{}d}.ppm", directory.string(), frame, suffix_width), res, pixels);
        initial /= zoom_per_frame;
    }
}

}   // namespace wacfrac
