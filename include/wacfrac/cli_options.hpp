#pragma once

#include "wacfrac/renderer.hpp"
#include <cstddef>
#include <string>
#include <optional>

namespace wacfrac {

struct VideoConfig {
    std::string directory {"mandelbrot"};
    double frames_per_second {24.0};
    std::size_t segment_size {64};
    MultiFloat initial_scale {0.4};
    MultiFloat final_scale {1e1};
    double zoom_per_second {2.0};
    bool do_preview {false};
};

struct CliOptions {
    enum class Mode { Image, Video };
    unsigned log_level {2};
    Mode mode;
    RendererConfig renderer;
    ImageConfig image;
    VideoConfig video;
};

auto parse_arguments(int argc, char* argv[]) -> std::optional<CliOptions>;

} // namespace wacfrac
