#include "wacfrac/gpu/host_interface.hpp"
#include "wacfrac/io.hpp"
#include "wacfrac/orbit.hpp"
#include "wacfrac/viewport.hpp"
#include "wacfrac/resolution.hpp"
#include "cuda/std/complex"
#include <print>

auto main() -> int {
    wacfrac::logging::init(0);
    std::println("Generating reference (n=256)");
    auto ref {wacfrac::compute_reference_mt<std::complex<double>>({-0.5, 0.0}, 256)}; // NOTE: enforcement of reference point fragile
    std::println("Generated {} sized reference", ref.size());
    std::println("Initializing renderer");
    wacfrac::Resolution res {1980, 1020};
    wacfrac::GpuRenderer renderer {0, res, wacfrac::ULTRA};
    std::println("Moving reference to GPU");
    renderer.copy_reference<std::complex<double>>(ref);
    std::println("Calling render func");
    auto pixels {renderer.render_perturbed<cuda::std::complex<double>>({{-0.5, 0.0}, 0.4, res}, 256, 16.0, false)};
    std::println("Saving to disk");
    wacfrac::write_ppm("gpubrot.ppm", res, pixels);
    return 0;
}
