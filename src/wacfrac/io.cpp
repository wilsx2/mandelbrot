#include "wacfrac/log.hpp"
#include "wacfrac/resolution.hpp"
#include <wacfrac/io.hpp>

#include <fstream>
#include <format>

namespace wacfrac
{

void write_ppm(std::string_view filename, resolution res, std::span<const pixel> pixels) {
    LOG_INFO << "Writing " << res.width << "x" << res.height << " image to \""
             << filename << "\" (" << pixels.size_bytes() << " bytes)";

    std::ofstream file(filename.data(), std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR << "Failed to open file for writing: " << filename;
        return;
    }

    auto header = std::format("P6\n{} {}\n255\n", res.width, res.height);
    file.write(header.data(), header.size());
    file.write(reinterpret_cast<const char*>(pixels.data()), pixels.size_bytes());
    LOG_INFO << "Image written successfully";
}

}   // namespace wacfrac
