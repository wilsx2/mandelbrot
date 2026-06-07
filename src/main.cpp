#include "mplot/mplot.hpp"
#include "argumentum/argparse.h"
#include <print>
#include <format>
#include <string>
#include <cstdlib>
#include <cstddef>

constexpr unsigned int MAX_ITERATIONS = 1000;

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
          .absent(MAX_ITERATIONS)
          .help("Maximum number of iterations per orbit (default: 1000)");


    std::vector<std::size_t> dimensions;
    params.add_parameter(dimensions, "--dimensions", "-d")
          .nargs(2)
          .absent({1920,1080})
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
          .absent({-0.75, 0.1})
          .help("Coordinates to zoom in on (default: -0.75 0.1 (seahorse valley))");

    double zoom_factor;
    params.add_parameter(zoom_factor, "--zoom-factor", "-z")
          .minargs(1)
          .absent({1.0})
          .help("How deeply to zoom in (default: 1.0)");

    std::size_t precision;
    params.add_parameter(precision, "--precision", "-p")
          .minargs(1)
          .absent(50uz)
          .help("Number of decimal digits to use to store floating point numbers (default: 50)");

    if (!parser.parse_args(argc, argv)) {
        std::exit(EXIT_FAILURE);
    }

    mplot::multi_float::default_precision(precision);
    mplot::multi_complex::default_precision(precision);
    
    std::println("Rendering plot to \"{}\"", filepath);
    std::println("\tDimensions: {}x{}", dimensions[0], dimensions[1]);
    std::println("\tReal Limits: min {}, max {}", real_limits[0], real_limits[1]);
    std::println("\tImaginary Limits: min {}i, max {}i", imag_limits[0], imag_limits[1]);
    std::println("\tFocusing On: {} {} {}i", focus[0], focus[1] >= 0 ? '+' : '-', focus[1]);
    std::println("\tZoom Factor(s): {}", zoom_factor);

    // Plot and render
    mplot::axis_limits l = {
        {real_limits[0], imag_limits[0]},
        {real_limits[1], imag_limits[1]}
    };
    l = l.at_zoom({focus[0], focus[1]}, zoom_factor);
    std::println("\tZoomed Real Limits: min {}, max {}", l.min.real().convert_to<double>(), l.max.real().convert_to<double>());
    std::println("\tZoomed Imaginary Limits: min {}i, max {}i", l.min.imag().convert_to<double>(), l.max.imag().convert_to<double>());
    bool success = mplot::save_plot(filepath, {
        .width = dimensions[0],
        .height = dimensions[1],
        .limits = l,
        .max_iterations = max_iterations
    });
    std::exit(success ? EXIT_SUCCESS : EXIT_FAILURE);
}
