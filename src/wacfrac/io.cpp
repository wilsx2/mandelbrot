#include "wacfrac/log.hpp"
#include "wacfrac/resolution.hpp"
#include <wacfrac/io.hpp>

#include <fstream>
#include <format>

namespace wacfrac
{

auto load_color_palette(std::string_view filename) -> std::vector<Pixel> {
    logging::print(logging::Severity::Trace, "Attempting to load colors from file {}", filename);
    std::ifstream file (filename.data());
    if (!file.is_open()) {
        logging::print(logging::Severity::Trace, "Failed to open file {}", filename);
        return {};
    }
    
    std::vector<Pixel> palette;
    std::string line;
    while (std::getline(file, line)) {
        if (line.size() < 7 || line[0] != '#') {
            logging::print(logging::Severity::Error, "Color parsing error on line '{}'", line);
            break;
        }
        std::stringstream ss {line.data() + 1};
        unsigned int hex_color;
        if (ss >> std::hex >> hex_color) {
            Pixel color {
                static_cast<uint8_t>((hex_color >> 16) & 0xFF),
                static_cast<uint8_t>((hex_color >> 8) & 0xFF),
                static_cast<uint8_t>(hex_color & 0xFF)
            };
            logging::print(logging::Severity::Debug, "Parsed line '{}' as rgb({},{},{})", line, color.r, color.g, color.b);
            palette.push_back(color);
        } else {
            logging::print(logging::Severity::Error, "Color parsing error on line '{}'", line);
            break;
        }
    }
    
    return palette;
}

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
