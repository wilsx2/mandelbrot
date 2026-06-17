#include <wacfrac/io.hpp>

#include <format>
#include <fstream>
#include <ranges>
#include <string>
#include <vector>

namespace wacfrac
{

auto save_to_ppm(std::string_view filename, resolution res, const std::span<pixel>& buffer) -> bool {
    if (res.width * res.height != buffer.size()) {
        return false;
    }

    std::ofstream file(filename.data(), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    auto header = std::format("P6\n{} {}\n255\n", res.width, res.height);
    file.write(header.data(), header.size());
    file.write(
        reinterpret_cast<const char*>(buffer.data()),
        static_cast<std::streamsize>(buffer.size_bytes())
    );

    return true;
}
auto save_to_ppm(std::string_view filename, const render_config& conf) -> bool {
    std::vector<pixel> buffer (conf.res.area());
    if (!render(conf, buffer))
        return false;
    return save_to_ppm(filename, conf.res, buffer);
}

// auto save_to_ppm(std::string_view filename, const std::vector<plot>& plots) -> bool {
//     std::vector<pixel> buffer;

//     for (const auto& [index, p] : std::views::enumerate(plots)) {
//         buffer.resize(p.res.area());

//         std::string num_string = std::string(std::floor(std::log10(plots.size())) - std::floor(std::log10(index)), '0') + std::to_string(index);
//         std::string final_filename = std::format("{}_{}.ppm", filename, num_string);
//         if (!save_to_ppm(final_filename, p)) {
//             return false;
//         }
//     }

//     return true;
// }

}   // namespace wacfrac
