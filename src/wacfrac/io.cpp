#include "wacfrac/resolution.hpp"
#include <wacfrac/io.hpp>

#include <fstream>
#include <format>

namespace wacfrac
{

void write_ppm(std::string_view filename, resolution res, std::span<const pixel> pixels) {
    std::ofstream file(filename.data(), std::ios::binary);
    if (!file.is_open()) return;

    auto header = std::format("P6\n{} {}\n255\n", res.width, res.height);
    file.write(header.data(), header.size());
    file.write(reinterpret_cast<const char*>(pixels.data()), pixels.size_bytes());
}

}   // namespace wacfrac
