#include <sstream>
#include <wacfrac/color.hpp>
#include <wacfrac/log.hpp>

namespace wacfrac
{

auto parse_color(std::string_view string) -> Pixel
{
    if (string.size() < 7 || string[0] != '#') {
        logging::error("Error parsing string as a color; size or prefix: {}", string);
        return {255, 0, 255};
    }

    std::stringstream ss{string.data() + 1};

    unsigned int hex_color;
    if (!(ss >> std::hex >> hex_color)) {
        logging::error("Error parsing string as a color; to hex: {}", string);
        return {255, 0, 255};
    }

    Pixel color{static_cast<uint8_t>((hex_color >> 16) & 0xFF), static_cast<uint8_t>((hex_color >> 8) & 0xFF),
                static_cast<uint8_t>(hex_color & 0xFF)};
    return color;
}

} // namespace wacfrac
