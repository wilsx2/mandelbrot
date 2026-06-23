#include "wacfrac/resolution.hpp"
#include <wacfrac/io.hpp>

#include <fstream>
#include <string_view>
#include <format>

namespace wacfrac
{

auto open_ppm(std::string_view filename, resolution res) -> std::ofstream {
    std::ofstream file(filename.data(), std::ios::binary);
    if (!file.is_open()) {
        return file;
    }

    auto header = std::format("P6\n{} {}\n255\n", res.width, res.height);
    file.write(header.data(), header.size());
    return file;
}

}   // namespace wacfrac
