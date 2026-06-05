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
          .absent("mandelbrot")
          .help("Path to output file (ommit file extension)");

    std::vector<int> dimensions;
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

    std::vector<double> zoom_factors;
    params.add_parameter(zoom_factors, "--zoom-factors", "-z")
          .minargs(1)
          .absent({1.0})
          .help("Factors of zoom to render (default: 1.0)");



    if (!parser.parse_args(argc, argv)) {
        std::exit(EXIT_FAILURE);
    }
    
    std::println("Rendering plot to \"{}\"", filepath);
    std::println("\tDimensions: {}x{}", dimensions[0], dimensions[1]);
    std::println("\tReal Limits: min {}, max {}", real_limits[0], real_limits[1]);
    std::println("\tImaginary Limits: min {}i, max {}i", imag_limits[0], imag_limits[1]);
    std::println("\tFocusing On: {} + {}i", focus[0], focus[1]);
    std::println("\tAt Zoom Factor(s): {:n}", zoom_factors);

    // Plot and render

    mplot::axis_limits lim {{real_limits[0], imag_limits[0]}, {real_limits[1], imag_limits[1]}};
    for (auto [index, factor] : std::views::enumerate(zoom_factors)) {
        auto final_filepath = std::format("{}{:05}.ppm", filepath, index);

        bool success = mplot::save_to_ppm(final_filepath, dimensions[0], dimensions[1], lim.at_zoom({focus[0], focus[1]}, factor),
            [](std::complex<double> c) -> mplot::pixel {
                auto percent = mplot::escape_time_percent(c, MAX_ITERATIONS); 
                if (percent == 1.0)
                    return {0, 0, 0};
                return mplot::hsv_to_rgb(percent, 1.0, 1.0);
            }
        );

        if (!success) {
            std::exit(EXIT_FAILURE);
        }
    }
    std::exit(EXIT_SUCCESS);
}
