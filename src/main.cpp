#include "wacfrac/bla.hpp"
#include "wacfrac/color.hpp"
#include "wacfrac/log.hpp"
#include "wacfrac/types.hpp"
#include "wacfrac/wacfrac.hpp"
#include "argumentum/argparse.h"
#include <cmath>
#include <string>
#include <cstdlib>
#include <cstddef>
#include <functional>
#include <chrono>
#include <atomic>

int main(int argc, char *argv[]) {
    wacfrac::logging::init();
    wacfrac::logging::print(wacfrac::logging::Severity::Info, "Mandelbrot Set Plotter starting");

    // Parse arguments
    auto parser = argumentum::argument_parser{};
    auto params = parser.params();

    if (argc < 1) {
        std::exit(EXIT_FAILURE);
    }
    parser.config().program(argv[0]).description("Mandelbrot Set Plotter");

    std::string filepath;
    params.add_parameter(filepath, "--output", "-o")
          .nargs(1)
          .absent("mandelbrot.ppm")
          .help("Path to output file (default: mandelbrot.ppm)");

    std::vector<std::size_t> dimensions;
    params.add_parameter(dimensions, "--dimensions", "-d")
          .nargs(2)
          .absent({500, 500})
          .help("Width and height of output image (default: 500 500)");

    std::vector<double> focus;
    params.add_parameter(focus, "--focus", "-f")
          .nargs(2)
          .absent({-0.5, 0})
          .help("Coordinates to zoom in on (default: -0.5 0)");

    std::string scale;
    params.add_parameter(scale, "--zoom-scale", "-z")
          .nargs(1)
          .absent({"0.4"})
          .help("Zoom scale (default: 0.4)");

    std::size_t max_iterations;
    params.add_parameter(max_iterations, "--max-iterations", "-n")
          .nargs(1)
          .absent({0})
          .help("Maximum number of iterations per orbit (default: approximate)");

    std::size_t precision;
    params.add_parameter(precision, "--precision", "-p")
          .nargs(1)
          .absent({0})
          .help("Number of decimal digits to use to store floating point numbers (default: approximate)");


    if (!parser.parse_args(argc, argv)) {
        std::exit(EXIT_FAILURE);
    }

    wacfrac::logging::print(wacfrac::logging::Severity::Info, "Configuration: output={} dimensions={}x{} focus=({}, {}) zoom={} max_iterations={} precision={}", filepath, dimensions[0], dimensions[1], focus[0], focus[1], scale, max_iterations, precision);

    wacfrac::MultiFloat zoom_scale (scale, 1000);

    // Plot and render
    wacfrac::Viewport view {
        wacfrac::poi::BIG_BANG,
        {1.0, 1.0}
    };
    view = view.zoomed(zoom_scale);

    if (max_iterations == 0)
        max_iterations = view.required_iterations();
    if (precision == 0)
        precision = view.required_precision();

    wacfrac::MultiFloat::default_precision(precision);
    wacfrac::MultiComplex::default_precision(precision);
    view.precision(precision);
    zoom_scale.precision(precision);

    wacfrac::Resolution res {dimensions[0], dimensions[1]};

    wacfrac::logging::print(wacfrac::logging::Severity::Info, "Viewport: center=({}) dimensions=({})", view.center, view.dimensions);
    wacfrac::logging::print(wacfrac::logging::Severity::Info, "Resolution: {}x{} ({} pixels)", res.width, res.height, res.area());
    wacfrac::logging::print(wacfrac::logging::Severity::Info, "Using {} max iterations with {} decimal digits precision", max_iterations, precision);

    wacfrac::MultiComplex c_ref = view.center;

    auto ref = wacfrac::compute_reference_mt<wacfrac::DoubleExpComplex>(c_ref, max_iterations);
    wacfrac::logging::print(wacfrac::logging::Severity::Info, "Reference at view center with orbit length {}", ref.size());

    auto t_render = std::chrono::steady_clock::now();

    std::vector<wacfrac::Pixel> pixels(res.area());

    wacfrac::logging::print(wacfrac::logging::Severity::Info, "Rendering {} pixels (perturbed)...", res.area());
    auto total_skipped = std::atomic<std::uint64_t>{0};

    auto last_level = static_cast<std::size_t>(std::log2(ref.size()));
    auto first_level = std::max(0uz, last_level > 9 ? last_level - 9 : 0uz);

    auto max_dc = wacfrac::to_complex<wacfrac::DoubleExpComplex>(view.compute_max_dc(c_ref));
    auto probes = view.generate_probes<wacfrac::DoubleExpComplex>(3, 3);
    wacfrac::BivariateLinearApproximator<wacfrac::DoubleExpComplex> bla {
        -255.0, -64.0, 1e-8,
        probes, max_dc, ref, first_level
    };
    wacfrac::perturbed_render<wacfrac::DoubleExpComplex>(
        pixels, res, view,
        [&bla, &total_skipped](auto dc) {
            auto result = bla.escape_approximate(dc);
            total_skipped += std::get<2>(result);
            return result;
        },
        [max_iterations](std::size_t n) {
            return wacfrac::colorize_unescaped(
                wacfrac::Pixel{0,0,0},
                std::bind_front(wacfrac::colorize_looped, wacfrac::palette::ULTRA),
                max_iterations, n
            );
        },
        c_ref
    );
    auto avg_skipped = static_cast<double>(total_skipped) / res.area();
    wacfrac::logging::print(wacfrac::logging::Severity::Info, "Perturbed render complete (avg skipped: {})", avg_skipped);

    auto render_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t_render);
    wacfrac::logging::print(wacfrac::logging::Severity::Info, "Render took {}ms", render_ms.count());

    wacfrac::write_ppm(filepath, res, pixels);
}
