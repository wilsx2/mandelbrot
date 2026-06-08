#pragma once

#include <wacfrac/plot.hpp>
#include <span>
#include <string_view>
#include <vector>

namespace wacfrac
{

auto save_to_ppm(std::string_view filename, resolution res, const std::span<pixel>& buffer) -> bool;
// TODO: Parameterize render function
auto save_to_ppm(std::string_view filename, const plot& p) -> bool;
auto save_to_ppm(std::string_view filename, const std::vector<plot>& plots) -> bool;

}   // namespace wacfrac
