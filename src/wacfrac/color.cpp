#include <sstream>
#include <vector>
#include <wacfrac/color.hpp>
#include <wacfrac/log.hpp>
#include <wacfrac/orbit.hpp>
#include <cmath>

namespace wacfrac
{

auto parse_color(std::string_view string) -> Pixel {
    if (string.size() < 7 || string[0] != '#') {
        logging::error( "Error parsing string as a color; size or prefix: {}", string);
        return {255,0,255};
    }

    std::stringstream ss {string.data() + 1};

    unsigned int hex_color;
    if (!(ss >> std::hex >> hex_color)) {
        logging::error( "Error parsing string as a color; to hex: {}", string);
        return {255,0,255};
    }

    Pixel color {
        static_cast<uint8_t>((hex_color >> 16) & 0xFF),
        static_cast<uint8_t>((hex_color >> 8) & 0xFF),
        static_cast<uint8_t>(hex_color & 0xFF)
    };
    logging::debug( "Parsed color '{}' as rgb({},{},{})", color.r, color.g, color.b);
    return color;
}

auto colorize_continuous(const std::vector<Pixel>& palette, std::size_t max_n, std::complex<float> z, std::size_t n) -> Pixel {
    if (n == max_n)
        return palette.back();
    auto cont_n {n - std::log(std::log(std::abs(z))) / std::log(2.0)};
    auto n1     {static_cast<std::size_t>(std::floor(cont_n))};
    auto n2     {static_cast<std::size_t>(std::ceil(cont_n))};
    auto color1 {palette.at(n1 % palette.size())};
    auto color2 {palette.at(n2 % palette.size())};
    auto progress {std::fmod(cont_n, 1.0)};
    return {
        static_cast<uint8_t>(color2.r * progress + color1.r * (1.0 - progress)),
        static_cast<uint8_t>(color2.g * progress + color1.g * (1.0 - progress)),
        static_cast<uint8_t>(color2.b * progress + color1.b * (1.0 - progress))
    };
}

auto colorize_discrete(const std::vector<Pixel>& palette, std::size_t max_n, std::size_t n) -> Pixel {
    if (n == max_n)
        return palette.back();
    return palette.at(n % palette.size());
}

}   // namespace wacfrac
