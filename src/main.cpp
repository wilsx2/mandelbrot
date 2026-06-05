#include "mplot/mplot.hpp"
#include "argumentum/argparse.h"
#include <print>
#include <format>
#include <string>
#include <cstdlib>
#include <cstddef>

constexpr unsigned int MAX_ITERATIONS = 255;

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
          .absent("output.ppm")
          .help("Path to the output file (.ppm format) (default: output.ppm)");

    std::array<int, 2> dimensions;
    params.add_parameter(dimensions, "--dimensions", "-d")
          .nargs(2)
          .absent({1920,1080})
          .help("Width and height of output image (default: 1920 1080)");

    std::array<double, 2> real_limits;
    params.add_parameter(real_limits, "--real-limits", "-r")
          .nargs(2)
          .absent({-2.0, 0.5})
          .help("Minimum and maximum values of the real component of 'c' to be graphed (default: -2.0 0.5)");

    std::array<double, 2> imag_limits;
    params.add_parameter(imag_limits, "--imaginary-limits", "-i")
          .nargs(2)
          .absent({-1.25, 1.25})
          .help("Minimum and maximum values of the imaginary component of 'c' to be graphed (default: -1.25 1.25)");

    if (!parser.parse_args(argc, argv)) {
        std::exit(EXIT_FAILURE);
    }
    
    std::println("Rendering plot to \"{}\"", filepath);
    std::println("\tDimensions: {}x{}", dimensions[0], dimensions[1]);
    std::println("\tReal Limits: min {}, max {}", real_limits[0], real_limits[1]);
    std::println("\tImaginary Limits: min {}, max {}", imag_limits[0], imag_limits[1]);

    // Plot and render
    mplot::axis_limits lim {{real_limits[0], imag_limits[0]}, {real_limits[1], imag_limits[1]}};
    bool success = mplot::save_to_ppm(filepath, dimensions[0], dimensions[1], lim,
        [](std::complex<double> c) -> mplot::pixel {
            auto percent = mplot::escape_time_percent(c, MAX_ITERATIONS); 
            if (percent == 1.0)
                return {0, 0, 0};
            return mplot::hsv_to_rgb(percent, 1.0, 1.0);
        }
    );
    std::exit(success ? EXIT_SUCCESS : EXIT_FAILURE);
}
