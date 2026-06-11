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

    std::vector<double> focus;
    params.add_parameter(focus, "--focus", "-f")
          .nargs(2)
          .absent({-2,0})
          .help("Coordinates to zoom in on (default: left-most tip)");

    double zoom_factor;
    params.add_parameter(zoom_factor, "--zoom-factor", "-z")
          .nargs(1)
          .absent({1.0})
          .help("How deeply to zoom in (default: 1.0)");

    unsigned int precision;
    params.add_parameter(precision, "--precision", "-p")
          .nargs(1)
          .absent(10u)
          .help("Number of decimal digits to use to store floating point numbers (default: 10)");

    bool purturbed;
    params.add_parameter(purturbed, "--purturbed", "-u")
          .nargs(1)
          .absent(false)
          .help("Use perturbation theory");

    if (!parser.parse_args(argc, argv)) {
        std::exit(EXIT_FAILURE);
    }

    wacfrac::multi_float::default_precision(precision);
    wacfrac::multi_complex::default_precision(precision);

    std::println("Rendering plot to \"{}\"", filepath);
    std::println("\tDimensions: {}x{}", dimensions[0], dimensions[1]);
    std::println("\tFocusing On: {} {} {}i", focus[0], focus[1] >= 0 ? '+' : '-', focus[1]);
    std::println("\tZoom Factor(s): {}", zoom_factor);

    // Plot and render
    wacfrac::viewport v {
        //wacfrac::poi::BIG_BANG,
        {focus[0], focus[1], precision},
        {2.5, 2.5, precision}
    };
    v.precision(precision);
    v.dimensions /= zoom_factor;

    wacfrac::plot p {
        .res            = {dimensions[0], dimensions[1]},
        .view           = v,
        .max_iterations = max_iterations
    };

    std::vector<wacfrac::pixel> buff (p.res.area());
    if (purturbed) {
        wacfrac::render_perturbed(p, buff);
    } else {
        wacfrac::render_directly(p, buff);
    }

    bool success = wacfrac::save_to_ppm(filepath, p.res, buff);
    std::exit(success ? EXIT_SUCCESS : EXIT_FAILURE);
}
