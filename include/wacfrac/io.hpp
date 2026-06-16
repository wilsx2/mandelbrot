#pragma once

#include <wacfrac/rendering.hpp>
#include <span>
#include <string_view>
#include <vector>

namespace wacfrac
{

auto save_to_ppm(std::string_view filename, resolution res, const std::span<pixel>& buffer) -> bool;
auto save_to_ppm(std::string_view filename, const render_config& conf) -> bool;

}   // namespace wacfrac
