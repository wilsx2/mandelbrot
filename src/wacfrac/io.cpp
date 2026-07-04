#include "wacfrac/io.hpp"
#include "wacfrac/log.hpp"
#include "wacfrac/resolution.hpp"
#include "wacfrac/types.hpp"
#include <cstdlib>
#include <fstream>
#include <format>
#include <span>

namespace wacfrac
{

auto write_ppm(std::filesystem::path filename, Resolution res, std::span<const Pixel> pixels) -> bool {
    logging::info( "Writing {}x{} image to \"{}\" ({} bytes)", res.width, res.height, filename, pixels.size_bytes());

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    auto header = std::format("P6\n{} {}\n255\n", res.width, res.height);
    file.write(header.data(), header.size());
    file.write(reinterpret_cast<const char*>(pixels.data()), pixels.size_bytes());
    return file.good();
}

auto total_frames(MultiFloat initial, MultiFloat target, MultiFloat zoom_per_second, float frames_per_second) -> std::size_t {
    using boost::multiprecision::log;
    using boost::multiprecision::exp;
    using boost::multiprecision::ceil;
    using boost::multiprecision::abs;

    auto zoom_per_frame {exp(abs(log(zoom_per_second)) / frames_per_second)};
    auto total {abs(log(initial / target))};
    return static_cast<std::size_t>(ceil(total / log(zoom_per_frame)));
}

auto frame_zooms(MultiFloat initial, MultiFloat target, MultiFloat zoom_per_second, float frames_per_second)
-> std::generator<MultiFloat> {
    using boost::multiprecision::log;
    using boost::multiprecision::exp;
    using boost::multiprecision::ceil;
    using boost::multiprecision::abs;

    auto zoom_per_frame {exp(abs(log(zoom_per_second)) / frames_per_second)};
    auto zoom_in = initial > target;
    while ((zoom_in && initial >= target) || (!zoom_in && initial <= target)) {
        co_yield initial;
        if (zoom_in) {
            initial /= zoom_per_frame;
        } else {
            initial *= zoom_per_frame;
        }
    }
}

auto file_suffix(std::size_t value, std::size_t max) -> std::string {
    return std::format("{:0{}d}", value, max > 0 ? std::to_string(max - 1).length() : 1);
} 
auto file_suffix_format(std::size_t max) -> std::string {
    return std::format("0{}d", max > 0 ? std::to_string(max - 1).length() : 1);
}

auto concatenate_images(std::filesystem::path output, std::string_view pattern, float fps) -> bool {
    return std::system(std::format(
        "ffmpeg -y -framerate {} -i {} -c:v libx264 -pix_fmt yuv420p {}", // TODO: Pipe into my stdout
        fps, pattern, output.string()
    ).c_str()) == EXIT_SUCCESS;
}
auto concatenate_videos(std::filesystem::path output, std::string_view pattern) -> bool {
    return std::system(std::format(
        "ffmpeg -f concat -safe 0 -i <(printf \"file '$PWD/%s'\n\" {}) -c copy {}",
        pattern, output.string()
    ).c_str()) == EXIT_SUCCESS;
}

}   // namespace wacfrac
