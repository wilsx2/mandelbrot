#include <wacfrac/io.hpp>

#include <format>
#include <fstream>
#include <ranges>
#include <string>
#include <vector>

namespace wacfrac
{

auto save_plot(std::string_view filename, const plot& p, const std::span<pixel>& buffer) -> bool {
    if (p.res.width * p.res.height != buffer.size()) {
        return false;
    }
    if (!p.render(buffer)) {
        return false;
    }

    std::ofstream file(filename.data(), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    auto header = std::format("P6\n{} {}\n255\n", p.res.width, p.res.height);
    file.write(header.data(), header.size());
    file.write(
        reinterpret_cast<const char*>(buffer.data()),
        static_cast<std::streamsize>(buffer.size_bytes())
    );

    return true;
}

auto save_plot(std::string_view filename, const plot& p) -> bool {
    std::vector<pixel> buffer(p.res.width * p.res.height);
    return save_plot(filename, p, buffer);
}

auto save_plots(std::string_view filename, const std::vector<plot>& plots) -> bool {
    std::vector<pixel> buffer;

    for (const auto& [index, p] : std::views::enumerate(plots)) {
        buffer.resize(p.res.width * p.res.height);

        std::string num_string = std::string((plots.size() + 9) / 10 - (index + 10) / 10, '0') + std::to_string(index);
        std::string final_filename = std::format("{}_{}.ppm", filename, num_string);
        if (!save_plot(final_filename, p, buffer)) {
            return false;
        }
    }

    return true;
}

}   // namespace wacfrac
