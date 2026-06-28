#include "wacfrac/log.hpp"
#include "wacfrac/resolution.hpp"
#include "wacfrac/types.hpp"
#include <wacfrac/io.hpp>

#include <fstream>
#include <format>
#include <span>

namespace wacfrac
{

void write_ppm(std::string_view filename, Resolution res, std::span<const Pixel> pixels) {
    logging::print(logging::Severity::Info, "Writing {}x{} image to \"{}\" ({} bytes)", res.width, res.height, filename, pixels.size_bytes());

    std::ofstream file(filename.data(), std::ios::binary);
    if (!file.is_open()) {
        logging::print(logging::Severity::Error, "Failed to open file for writing: {}", filename);
        return;
    }

    auto header = std::format("P6\n{} {}\n255\n", res.width, res.height);
    file.write(header.data(), header.size());
    file.write(reinterpret_cast<const char*>(pixels.data()), pixels.size_bytes());
    logging::print(logging::Severity::Info, "Image written successfully");
}

}   // namespace wacfrac
