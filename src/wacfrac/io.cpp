#include "wacfrac/io.hpp"
#include "wacfrac/log.hpp"
#include "wacfrac/resolution.hpp"
#include "wacfrac/types.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <format>
#include <random>
#include <span>
#include <vector>

namespace wacfrac
{

auto write_ppm(std::filesystem::path filename, Resolution res, std::span<const Pixel> pixels) -> bool {
    logging::info( "Writing {}x{} image to {} ({} bytes) ptr={} first=({},{},{})",
        res.width, res.height, filename, pixels.size_bytes(), (void*)pixels.data(),
        pixels.size() > 0 ? static_cast<int>(pixels[0].r) : 0,
        pixels.size() > 0 ? static_cast<int>(pixels[0].g) : 0,
        pixels.size() > 0 ? static_cast<int>(pixels[0].b) : 0);

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
    auto star = pattern.find('*');
    if (star == std::string_view::npos)
        return false;

    std::string prefix(pattern.substr(0, star));
    std::string suffix(pattern.substr(star + 1));

    std::vector<std::filesystem::path> matches;
    for (auto& entry : std::filesystem::directory_iterator("."))
        if (auto name = entry.path().filename().string();
            name.starts_with(prefix) && name.ends_with(suffix))
            matches.push_back(std::filesystem::absolute(entry.path()));
    std::ranges::sort(matches);

    auto tmp = std::filesystem::temp_directory_path() / std::format("wacfrac_concat_{}.txt", std::random_device{}());
    std::ofstream file(tmp);
    if (!file.is_open())
        return false;
    for (auto& path : matches)
        file << "file '" << path.string() << "'\n";
    file.close();

    auto result = std::system(std::format(
        "ffmpeg -f concat -safe 0 -i \"{}\" -c copy \"{}\"",
        tmp.string(), output.string()
    ).c_str()) == EXIT_SUCCESS;

    std::filesystem::remove(tmp);
    return result;
}

}   // namespace wacfrac
