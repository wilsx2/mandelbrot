#pragma once
#include <wacfrac/types.hpp>
#include <wacfrac/plot.hpp>

namespace wacfrac {

constexpr std::initializer_list<std::initializer_list<double>> FULL_SET = {
    {-2.5, -1.0},
    {+1.0, +1.0}
};

namespace video_resolution {
    constexpr auto SD360p  = {640uz, 360uz};
    constexpr auto SD480p  = {853uz, 480uz};
    constexpr auto HD720p  = {1280uz, 720uz};
    constexpr auto HD1080p = {1920uz, 1080uz};
    constexpr auto UHD4K   = {3840uz, 2160uz};
} // namespace video_resolution

namespace poi {
    // https://www.mrob.com/pub/muency/rationalcoordinates.html
    constexpr auto UTTER_WEST = {-2.0, 0.0};
    constexpr auto SEAHORSE_VALLEY = {0.25, 0.0};
    constexpr auto QUAD_SPIRAL_VALLEY = {0.25, 0.5};
} // namespace poi

} // namespace wacfrac
