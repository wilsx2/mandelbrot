#pragma once

#include "wacfrac/color.hpp"
#include "wacfrac/resolution.hpp"
#include "wacfrac/types.hpp"

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace wacfrac
{

    // image
    auto write_ppm(std::filesystem::path filename, Resolution res, std::span<const Pixel> pixels) -> bool;

    // video
    auto total_frames(MultiFloat initial, MultiFloat target, MultiFloat zoom_per_second, float frames_per_second)
        -> std::size_t;
    auto frame_zooms(MultiFloat initial, MultiFloat target, MultiFloat zoom_per_second, float frames_per_second)
        -> std::vector<MultiFloat>;
    auto file_suffix(std::size_t value, std::size_t max) -> std::string; // {:0{}d}
    auto file_suffix_format(std::size_t max) -> std::string;

    // ffmpeg
    auto concatenate_images(std::filesystem::path output, std::string_view pattern, float fps) -> bool;
    auto concatenate_videos(std::filesystem::path output, std::string_view pattern) -> bool;

} // namespace wacfrac
