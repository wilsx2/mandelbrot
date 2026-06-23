#include "wacfrac/bla.hpp"
#include "wacfrac/color.hpp"
#include "wacfrac/log.hpp"
#include "wacfrac/types.hpp"
#include "wacfrac/wacfrac.hpp"
#include "argumentum/argparse.h"
#include <format>
#include <string>
#include <cstdlib>
#include <cstddef>
#include <functional>
#include <chrono>
#include <atomic>

int main(int argc, char *argv[]) {
    wacfrac::logging::init();
    LOG_INFO << "Mandelbrot Set Plotter starting";

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

    LOG_INFO << "Configuration: output=" << filepath
             << " dimensions=" << dimensions[0] << "x" << dimensions[1]
             << " focus=(" << focus[0] << ", " << focus[1] << ")"
             << " zoom=" << scale
             << " max_iterations=" << max_iterations
             << " precision=" << precision;

    wacfrac::multi_float zoom_scale (scale, 1000);

    // Plot and render
    wacfrac::viewport view {
        wacfrac::poi::BIG_BANG,
        //{focus[0], focus[1], precision},
        {1.0, 1.0}
    };
    view = view.zoomed(zoom_scale);

    if (max_iterations == 0)
        max_iterations = view.required_iterations();
    if (precision == 0)
        precision = view.required_precision();

    wacfrac::multi_float::default_precision(precision);
    wacfrac::multi_complex::default_precision(precision);
    zoom_scale.precision(precision);
    view.precision(precision);

    wacfrac::resolution res {dimensions[0], dimensions[1]};

    LOG_INFO << "Viewport: center=(" << view.center << ") dimensions=(" << view.dimensions << ")";
    LOG_INFO << "Resolution: " << res.width << "x" << res.height
             << " (" << res.area() << " pixels)";
    LOG_INFO << "Using " << max_iterations << " max iterations with "
             << precision << " decimal digits precision";

    auto t_start = std::chrono::steady_clock::now();

    auto [c_ref, ref] = view.find_periodic_reference<std::complex<double>>(max_iterations, 64uz, 64uz);
    LOG_INFO << "Found periodic reference at (" << c_ref << ") with orbit length " << ref.size();
    wacfrac::bivariate_linear_approximator<std::complex<double>> bla {
        1e-30, 1e-6,
        view.generate_probes<std::complex<double>>(4, 4),
        wacfrac::to_complex<std::complex<double>>(view.compute_max_dc(c_ref)),
        ref, 0
    };

    std::vector<wacfrac::pixel> pixels(res.area());
    LOG_INFO << "Rendering " << res.area() << " pixels...";
    auto total_skipped = std::atomic<std::uint64_t>{0};
    wacfrac::perturbed_render<std::complex<double>>(
        pixels, res, view,
        [&bla, &total_skipped](auto dc) {
            auto result = bla.escape_approximate(dc);
            total_skipped += std::get<2>(result);
            return result;
        },
        [max_iterations](std::size_t n) {
            return wacfrac::colorize_unescaped(
                wacfrac::pixel{0,0,0},
                std::bind_front(wacfrac::colorize_looped, wacfrac::palette::ultra),
                max_iterations, n
            );
        },
        c_ref
    );
    auto t_render = std::chrono::steady_clock::now();
    auto avg_skipped = static_cast<double>(total_skipped) / res.area();
    LOG_INFO << "Render complete in "
             << std::chrono::duration_cast<std::chrono::milliseconds>(t_render - t_start).count()
             << "ms (avg skipped: " << avg_skipped << ")";

    wacfrac::write_ppm(filepath, res, pixels);
}
