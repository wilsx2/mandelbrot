#include "wacfrac/gpu_renderer.hpp"
#include "wacfrac/io.hpp"
#include "wacfrac/orbit.hpp"
#include "wacfrac/reference.hpp"
#include "wacfrac/viewport.hpp"
#include "wacfrac/resolution.hpp"
#include <print>

auto main() -> int {
    wacfrac::logging::init(0);
    std::println("Generating references (n=256)");
    auto ref_set {wacfrac::compute_reference_set({-0.5, 0.0}, 256)}; // NOTE: enforcement of reference point fragile
    std::println("Initializing renderer");
    wacfrac::Resolution res {1980, 1020};
    wacfrac::GpuRenderer renderer {0, res, wacfrac::ULTRA};
    std::println("Moving reference to GPU");
    renderer.copy_references(ref_set);
    std::println("Calling render func");
    auto pixels {renderer.render_perturbed<wacfrac::DoubleComplex>({{-0.5, 0.0}, 0.4, res}, 256)};
    std::println("Saving to disk");
    wacfrac::write_ppm("gpubrot.ppm", res, pixels);
    return 0;
}
