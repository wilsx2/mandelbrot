#include "wacfrac/bla.hpp"
#include "wacfrac/color.hpp"
#include "wacfrac/types.hpp"
#include "wacfrac/wacfrac.hpp"
#include "argumentum/argparse.h"
#include <print>
#include <format>
#include <string>
#include <cstdlib>
#include <cstddef>
#include <functional>

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

    std::println("Rendering plot to \"{}\"", filepath);

    wacfrac::resolution res {dimensions[0], dimensions[1]};
    auto file = wacfrac::open_ppm(filepath, res);
    if (!file.is_open()) {
        std::exit(EXIT_FAILURE);
    }

    auto [c_ref, ref] = view.find_periodic_reference<wacfrac::doubleexp_complex>(max_iterations, max_iterations, 256uz);
    wacfrac::bivariate_linear_approximator<wacfrac::doubleexp_complex> bla { 1e-30, 1e-3, ref, 0 };
    wacfrac::perturbed_render<wacfrac::doubleexp_complex>(
        file, res, view,
        [&](wacfrac::doubleexp_complex dc){ return bla.escape_approximate(dc); },
        [&](wacfrac::doubleexp_complex z, std::size_t n) -> wacfrac::pixel {
            if (n == max_iterations) {
                return {0,0,0};
            }
            
            auto lookup = std::bind_front(wacfrac::colorize_looped, wacfrac::palette::ultra);
            return wacfrac::colorize_continuous(std::move(lookup),
                std::complex<float>{static_cast<float>(z.real()), static_cast<float>(z.imag())},
                n
            );
            //(void) z;
            //return wacfrac::colorize_looped(wacfrac::palette::ultra, n);
        },
        c_ref
    );
}
