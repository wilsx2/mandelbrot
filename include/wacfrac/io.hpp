#pragma once

#include "wacfrac/color.hpp"
#include "wacfrac/log.hpp"
#include "wacfrac/resolution.hpp"
#include "wacfrac/types.hpp"
#include <algorithm>
#include <concepts>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <sstream>
#include <wacfrac/rendering.hpp>
#include <span>
#include <string>
#include <string_view>

namespace wacfrac
{

void write_ppm(std::string_view filename, Resolution res, std::span<const Pixel> pixels);

#define CHUNK_SIZE 16
template <std::invocable<std::span<Pixel>, MultiFloat> F>
void write_zoom_frames(std::filesystem::path directory, Resolution res, MultiFloat initial, MultiFloat target, MultiFloat zoom_per_second, float frames_per_second, F&& render_at_scale) {
    std::filesystem::create_directories(directory);
    std::filesystem::current_path(directory);

    if (std::filesystem::exists("final.mp4")) {
        logging::print(logging::Severity::Info, "'{}' already contains a complete render", directory);
        return;
    }

    using boost::multiprecision::log;
    using boost::multiprecision::exp;
    using boost::multiprecision::ceil;
    using boost::multiprecision::abs;
    auto zoom_per_frame {exp(abs(log(zoom_per_second)) / frames_per_second)};
    auto total {abs(log(initial / target))};
    auto frames {static_cast<std::size_t>(ceil(total / log(zoom_per_frame)))};
    auto suffix_width = frames > 0 ? std::to_string(frames - 1).length() : 1; // TODO: Update to match digits of max(segments, frames in segment)
    std::vector<Pixel> pixels (res.area());
    auto zoom_in = initial > target;
    for (auto frame : std::views::iota(0uz, frames)) {
        auto segment = frame / CHUNK_SIZE;
        std::string frame_filename {std::format("frame_{:0{}d}.ppm", frame % CHUNK_SIZE, suffix_width)};
        std::string segment_filename {std::format("segment_{:0{}d}.mp4", segment, suffix_width)};

        if (!std::filesystem::exists(segment_filename)) {
            if (!std::filesystem::exists(frame_filename)) {
                render_at_scale(pixels, initial);
                write_ppm(frame_filename, res, pixels);
            }

            if (frame != 0 && (frame % CHUNK_SIZE == CHUNK_SIZE - 1 || frame == frames - 1)) {
                logging::print(logging::Severity::Debug, "Composing frames into segment {}", segment);
                auto status = std::system(std::format(
                    "ffmpeg -y -framerate {} -i frame_%0{}d.ppm -c:v libx264 -pix_fmt yuv420p {}", // TODO: Pipe into my stdout
                    frames_per_second, suffix_width, segment_filename
                ).c_str());
                if (status == EXIT_SUCCESS) {
                    std::system("rm frame_*");
                    logging::print(logging::Severity::Info, "Segment #{} rendered", segment);
                } else {
                    logging::print(logging::Severity::Error, "Segment #{} failed to compose", segment);
                }
            }
        } else {
            logging::print(logging::Severity::Debug, "Frame #{} has already been rendered; skipping", frame);
        }

        if (zoom_in) {
            initial /= zoom_per_frame;
        } else {
            initial *= zoom_per_frame;
        }
    }
    auto status = std::system(std::format(
        "ffmpeg -f concat -safe 0 -i <(printf \"file '$PWD/%s'\n\" segment_*.mp4) -c copy final.mp4",
        suffix_width
    ).c_str());
    if (status == EXIT_SUCCESS) {
        (void) std::system("rm segment_*");
        logging::print(logging::Severity::Info, "Video render complete");
    } else {
        logging::print(logging::Severity::Error, "Final video failed to compose");
    }
}

}   // namespace wacfrac
