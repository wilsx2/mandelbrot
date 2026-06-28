#pragma once

#include <argumentum/argparse.h>
#include <cstddef>
#include <string>
#include <vector>

namespace wacfrac {

struct SharedOptions {
    std::string filepath {"mandelbrot.ppm"};
    std::vector<std::size_t> dimensions {500, 500};
    std::vector<double> focus {-0.5, 0.0};
    std::string scale {"0.4"};
    std::size_t max_iterations {0};
    std::size_t precision {0};
    double escape_radius {2.0};
    std::string numeric_type {"double"};
    bool continuous_coloring {false};

    void add_to(argumentum::ParameterConfig& args) {
        args.add_parameter(filepath, "--output", "-o")
            .nargs(1).absent("mandelbrot.ppm")
            .help("Path to output file");
        args.add_parameter(dimensions, "--dimensions", "-d")
            .nargs(2).absent({500, 500})
            .help("Width and height of output image");
        args.add_parameter(focus, "--focus", "-f")
            .nargs(2).absent({-0.5, 0.0})
            .help("Coordinates to zoom in on");
        args.add_parameter(scale, "--zoom-scale", "-z")
            .nargs(1).absent("0.4")
            .help("Zoom scale factor");
        args.add_parameter(max_iterations, "--max-iterations", "-n")
            .nargs(1).absent(0)
            .help("Maximum iterations (0 = auto)");
        args.add_parameter(precision, "--precision", "-p")
            .nargs(1).absent(0)
            .help("Decimal digits (0 = auto)");
        args.add_parameter(escape_radius, "--escape-radius")
            .nargs(1).absent(2.0)
            .help("Escape radius");
        args.add_parameter(numeric_type, "--numeric-type")
            .nargs(1).absent("double")
            .help("Number type: double, long-double, dexp");
        args.add_parameter(continuous_coloring, "--continuous-coloring")
            .nargs(0).absent(false)
            .help("Enable smooth/continuous coloring");
    }
};

struct DirectOptions : argumentum::CommandOptions {
    using CommandOptions::CommandOptions;
    std::shared_ptr<SharedOptions> shared;

    void add_parameters(argumentum::ParameterConfig& args) override {
        shared = std::make_shared<SharedOptions>();
        shared->add_to(args);
    }
};

struct PerturbedOptions : argumentum::CommandOptions {
    using CommandOptions::CommandOptions;
    std::shared_ptr<SharedOptions> shared;

    void add_parameters(argumentum::ParameterConfig& args) override {
        shared = std::make_shared<SharedOptions>();
        shared->add_to(args);
    }
};

struct SAOptions : argumentum::CommandOptions {
    using CommandOptions::CommandOptions;
    std::shared_ptr<SharedOptions> shared;

    std::size_t coeffs {0};
    std::vector<std::size_t> probes {3, 3};
    double validity_threshold {1e-6};
    std::size_t n {0};

    void add_parameters(argumentum::ParameterConfig& args) override {
        shared = std::make_shared<SharedOptions>();
        shared->add_to(args);

        args.add_parameter(coeffs, "--coeffs", "-c")
            .nargs(1).required(true)
            .help("Number of SA coefficients");
        args.add_parameter(probes, "--probes")
            .nargs(2).absent({3, 3})
            .help("Probe grid dimensions (rows cols)");
        args.add_parameter(validity_threshold, "--validity-threshold")
            .nargs(1).absent(1e-6)
            .help("Validity threshold for probe-based SA");
        args.add_parameter(n, "--n")
            .nargs(1).absent(0)
            .help("Iterations for direct SA (0 = use probe-based validity)");
    }
};

struct BLAOptions : argumentum::CommandOptions {
    using CommandOptions::CommandOptions;
    std::shared_ptr<SharedOptions> shared;

    std::vector<std::size_t> probes {3, 3};
    double tolerance {1e-8};
    int lower_exp {-(1 << 12)};
    int upper_exp {-(1 << 0)};
    double epsilon {0.0};
    std::size_t first_level {0};

    void add_parameters(argumentum::ParameterConfig& args) override {
        shared = std::make_shared<SharedOptions>();
        shared->add_to(args);

        args.add_parameter(probes, "--probes")
            .nargs(2).absent({3, 3})
            .help("Probe grid dimensions (rows cols)");
        args.add_parameter(tolerance, "--tolerance")
            .nargs(1).absent(1e-8)
            .help("Epsilon search tolerance");
        args.add_parameter(lower_exp, "--lower-exp")
            .nargs(1).absent(-(1 << 12))
            .help("Lower exponent for epsilon search");
        args.add_parameter(upper_exp, "--upper-exp")
            .nargs(1).absent(-(1 << 0))
            .help("Upper exponent for epsilon search");
        args.add_parameter(epsilon, "--epsilon")
            .nargs(1).absent(0.0)
            .help("Direct epsilon value (0 = use binary search)");
        args.add_parameter(first_level, "--first-level")
            .nargs(1).absent(0)
            .help("First BLA level (0 = auto)");
    }
};

} // namespace wacfrac
