#include "gpu_test.hpp"
#include "wacfrac/io.hpp"
#include "wacfrac/viewport.hpp"
#include "wacfrac/resolution.hpp"

auto main() -> int {
    wacfrac::Resolution res {1980, 120};
    wacfrac::Viewport view {-0.5, 2.5};
    std::vector<wacfrac::Pixel> pixels (res.area());
    gpu_rendering_pass(pixels, res, view);
    wacfrac::write_ppm("gpubrot.ppm", res, pixels);
    return 0;
}
