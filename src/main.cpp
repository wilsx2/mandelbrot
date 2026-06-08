#include "wacfrac/wacfrac.hpp"
#include "argumentum/argparse.h"
#include <print>
#include <format>
#include <string>
#include <cstdlib>
#include <cstddef>

int main(int argc, char *argv[]) {
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

    unsigned int max_iterations;
    params.add_parameter(max_iterations, "--max-iterations", "-n")
          .nargs(1)
          .absent(256)
          .help("Maximum number of iterations per orbit (default: 256)");

    std::vector<std::size_t> dimensions;
    params.add_parameter(dimensions, "--dimensions", "-d")
          .nargs(2)
          .absent({1920, 1080})
          .help("Width and height of output image (default: 1920 1080)");

    std::vector<double> real_limits;
    params.add_parameter(real_limits, "--real-limits", "-r")
          .nargs(2)
          .absent({-2.0, 0.5})
          .help("Minimum and maximum values of the real component of 'c' to be graphed (default: -2.0 0.5)");

    std::vector<double> imag_limits;
    params.add_parameter(imag_limits, "--imaginary-limits", "-i")
          .nargs(2)
          .absent({-1.25, 1.25})
          .help("Minimum and maximum values of the imaginary component of 'c' to be graphed (default: -1.25 1.25)");

    std::vector<double> focus;
    params.add_parameter(focus, "--focus", "-f")
          .nargs(2)
          .absent({-0.7269, 0.1889})
          .help("Coordinates to zoom in on (default: -0.7269 0.1889 (Misiurewicz point, seahorse valley tip))");

    double zoom_factor;
    params.add_parameter(zoom_factor, "--zoom-factor", "-z")
          .minargs(1)
          .absent({1.0})
          .help("How deeply to zoom in (default: 1.0)");

    unsigned int precision;
    params.add_parameter(precision, "--precision", "-p")
          .minargs(1)
          .absent(10u)
          .help("Number of decimal digits to use to store floating point numbers (default: 10)");

    if (!parser.parse_args(argc, argv)) {
        std::exit(EXIT_FAILURE);
    }

    wacfrac::multi_float::default_precision(precision);
    wacfrac::multi_complex::default_precision(precision);

    std::println("Rendering plot to \"{}\"", filepath);
    std::println("\tDimensions: {}x{}", dimensions[0], dimensions[1]);
    std::println("\tReal Limits: min {}, max {}", real_limits[0], real_limits[1]);
    std::println("\tImaginary Limits: min {}i, max {}i", imag_limits[0], imag_limits[1]);
    std::println("\tFocusing On: {} {} {}i", focus[0], focus[1] >= 0 ? '+' : '-', focus[1]);
    std::println("\tZoom Factor(s): {}", zoom_factor);

    // Plot and render
    wacfrac::viewport v = {
        {real_limits[0], imag_limits[0], precision},
        {real_limits[1], imag_limits[1], precision}
    };
    v = v.at_zoom({focus[0], focus[1]}, zoom_factor);
    std::println("\tZoomed Real Limits: min {}, max {}", v.min.real().convert_to<double>(), v.max.real().convert_to<double>());
    std::println("\tZoomed Imaginary Limits: min {}i, max {}i", v.min.imag().convert_to<double>(), v.max.imag().convert_to<double>());

    bool success = wacfrac::save_to_ppm(filepath, {
        .res            = {dimensions[0], dimensions[1]},
        .view           = v,
        .max_iterations = max_iterations
    });
    std::exit(success ? EXIT_SUCCESS : EXIT_FAILURE);
}
