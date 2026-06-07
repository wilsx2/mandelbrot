#pragma once

#include <cstdint>

namespace wacfrac
{

struct pixel { uint8_t r, g, b; };

auto hsv_to_rgb(float h, float s, float v) -> pixel;

// TODO: implement LCH (perceptual) -> sRGB conversion
auto lch_to_rgb(float l, float c, float h) -> pixel;

}   // namespace wacfrac
