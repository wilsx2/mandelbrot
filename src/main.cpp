#include "mplot/mplot.h"
#include <print>
#include <format>
#include <string>
#include <cstdlib>
#include <cstddef>

constexpr unsigned int MAX_ITERATIONS = 255;

int main(int argc, char *argv[]) {
    if (argc != 8) {
        std::println(stderr, "Expected 7 arguments, received {}", argc - 1);
        std::exit(EXIT_FAILURE);
    }

    // Parse arguments
    std::string filename = std::format("{}.ppm", std::string_view(argv[1]));
    std::size_t width    = std::stoull(argv[2]);
    std::size_t height   = std::stoull(argv[3]);
    double real_max      = std::stod(argv[4]);
    double real_min      = std::stod(argv[5]);
    double imag_max      = std::stod(argv[6]);
    double imag_min      = std::stod(argv[7]);

    // Plot and render
    mplot::axis_limits lim {{real_min, imag_min}, {real_max, imag_max}};
    bool success = mplot::save_to_ppm(filename, width, height, lim,
        [](std::complex<double> c) -> mplot::pixel {
            auto percent = mplot::escape_time_percent(c, MAX_ITERATIONS); 
            if (percent == 1.0)
                return {0, 0, 0};
            return mplot::hsv_to_rgb(percent, 1.0, 1.0);
        }
    );
    std::exit(success ? EXIT_SUCCESS : EXIT_FAILURE);
}
