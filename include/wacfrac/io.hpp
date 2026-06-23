#pragma once

#include <wacfrac/rendering.hpp>
#include <span>
#include <string_view>

namespace wacfrac
{

void write_ppm(std::string_view filename, resolution res, std::span<const pixel> pixels);

}   // namespace wacfrac
