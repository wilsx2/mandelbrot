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

    unsigned int max_iterations;
    params.add_parameter(max_iterations, "--max-iterations", "-n")
          .nargs(1)
          .absent({0})
          .help("Maximum number of iterations per orbit (default: approximate)");

    unsigned int precision;
    params.add_parameter(precision, "--precision", "-p")
          .nargs(1)
          .absent({0})
          .help("Number of decimal digits to use to store floating point numbers (default: approximate)");


    if (!parser.parse_args(argc, argv)) {
        std::exit(EXIT_FAILURE);
    }
    wacfrac::multi_float zoom_scale (scale, 1000);

    // Plot and render
    wacfrac::viewport v {
        wacfrac::poi::BIG_BANG,
        //{focus[0], focus[1], precision},
        {1.0, 1.0}
    };
    v = v.zoomed(zoom_scale);

    if (max_iterations == 0)
        max_iterations = v.required_iterations();
    if (precision == 0)
        precision = v.required_precision();

    wacfrac::multi_float::default_precision(precision);
    wacfrac::multi_complex::default_precision(precision);
    zoom_scale.precision(precision);
    v.precision(precision);

    std::println("Rendering plot to \"{}\"", filepath);

    wacfrac::render_config conf {
        .res            = {dimensions[0], dimensions[1]},
        .view           = v,
        .max_iterations = max_iterations,
        .palette        = wacfrac::generate_palette(32, wacfrac::color_encoding::hcl, {{.9f,.8f,.1f}, {1.f,1.f,1.f}, {.7f,.5f,.5f}}),
        .eta            = wacfrac::bla_eta {
            .epsilon =  1e-2,
            .first_level = 0
        },
        .ca             = {
            wacfrac::colorization_type::continuous,
            wacfrac::colorization_method::looped
        }
    };

    bool success = wacfrac::save_to_ppm(filepath, conf);
    std::exit(success ? EXIT_SUCCESS : EXIT_FAILURE);
}
