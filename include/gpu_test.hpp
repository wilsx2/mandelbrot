#pragma once

#include "wacfrac/resolution.hpp"
#include "wacfrac/viewport.hpp"
#include "wacfrac/wacfrac.hpp"
#include <span>

void gpu_rendering_pass(std::span<wacfrac::Pixel> pixels,
                        wacfrac::Resolution res,
                        wacfrac::Viewport view);
