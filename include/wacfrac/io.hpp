#pragma once

#include <wacfrac/rendering.hpp>
#include <fstream>
#include <string_view>

namespace wacfrac
{

auto open_ppm(std::string_view filename, resolution res) -> std::ofstream;

}   // namespace wacfrac
