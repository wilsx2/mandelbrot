#pragma once
#include <wacfrac/types.hpp>
#include <wacfrac/plot.hpp>
#include <wacfrac/viewport.hpp>

namespace wacfrac {

inline const viewport FULL_SET = {
    multi_complex(-2.5, -1.0),
    multi_complex(+1.0, +1.0)
};

namespace video_resolution {
    inline constexpr resolution SD360p  = {640,  360};
    inline constexpr resolution SD480p  = {853,  480};
    inline constexpr resolution HD720p  = {1280, 720};
    inline constexpr resolution HD1080p = {1920, 1080};
    inline constexpr resolution UHD4K   = {3840, 2160};
} // namespace video_resolution

namespace poi {
    // https://www.mrob.com/pub/muency/rationalcoordinates.html
    inline const multi_complex UTTER_WEST         = {-2.0,  0.0};
    inline const multi_complex SEAHORSE_VALLEY    = {0.25,  0.0};
    inline const multi_complex QUAD_SPIRAL_VALLEY = {0.25,  0.5};
} // namespace poi

} // namespace wacfrac
