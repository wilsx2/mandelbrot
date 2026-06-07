#pragma once

#include <wacfrac/plot.hpp>
#include <span>
#include <string_view>
#include <vector>

namespace wacfrac
{

auto save_plot(std::string_view filename, const plot& p, const std::span<pixel>& buffer) -> bool;
auto save_plot(std::string_view filename, const plot& p) -> bool;
auto save_plots(std::string_view filename, const std::vector<plot>& plots) -> bool;

}   // namespace wacfrac
