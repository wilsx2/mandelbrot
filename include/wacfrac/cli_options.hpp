#pragma once

#include "wacfrac/resolution.hpp"
#include "wacfrac/types.hpp"
#include "wacfrac/log.hpp"
#include "wacfrac/constants.hpp"
#include "wacfrac/color.hpp"
#include "wacfrac/viewport.hpp"
#include <argumentum/argparse.h>
#include <argumentum/inc/optionpack.h>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <ranges>

namespace wacfrac {

static void parse_multifloat(MultiFloat& target, const std::string& value){
    target = MultiFloat(value); // NOTE: Default precision is unset to its possible we get a truncation here
}

static void parse_probes_pair(std::pair<std::size_t, std::size_t>& target, const std::string& value){
    std::stringstream ss (value);
    ss >> target.first;
    ss >> target.second;
}

struct SharedOptions {
    Resolution resolution {500, 500};
    MultiComplex focus {0.0};
    double escape_radius {2.0};
    std::vector<Pixel> palette {};
    bool continuous_coloring {false};

    void add_to(argumentum::ParameterConfig& args) {
        args.add_parameter(resolution, "--resolution", "-r")
            .nargs(2).absent({500, 500})
            .action([&](auto& target, const std::string& value){
                std::stringstream ss (value);
                ss >> target.width;
                ss >> target.height;
            })
            .help("Width and height of output image");
        args.add_parameter(focus, "--focus", "-f")
            .nargs(2).absent({-0.5, 0.0})
            .action([&](auto& target, const std::string& value){
                auto segments = value | std::views::split(',');
                auto it = segments.begin();
                std::string real_str((*it).begin(), (*it).end());
                ++it;
                std::string imag_str((*it).begin(), (*it).end());
                target = MultiComplex{MultiFloat{real_str, 2000}, MultiFloat{imag_str, 2000}};
            })
            .help("Coordinates to zoom in on");
        args.add_parameter(escape_radius, "--escape-radius")
            .nargs(1).absent(2.0)
            .help("Escape radius");
        args.add_parameter(palette, "--palette")
            .minargs(0).absent(palette::ULTRA)
            .action([&](auto& target, const std::string& value){
                std::ranges::copy(
                    value | std::views::split(' ') | std::views::transform([](auto subrange) {
                        return parse_color(std::string_view(&*subrange.begin(), subrange.size()));
                    }),
                    std::back_inserter(target)
                );
                if (target.empty()) {
                    logging::print(logging::Severity::Info,
                        "Falling back to default palette");
                    target = wacfrac::palette::ULTRA;
                }
            })
            .help("Hex formatted colors mapped to escape time");
        args.add_parameter(continuous_coloring, "--continuous-coloring")
            .nargs(0).absent(false)
            .help("Enable smooth/continuous coloring");
    }
};

struct ImageOptions : public SharedOptions {
    std::string filepath {"mandelbrot.ppm"};
    MultiFloat scale {0.4};
    std::size_t max_iterations {0};
    std::size_t precision {0};
    std::string numeric_type {"double"};

    void add_to(argumentum::ParameterConfig& args) {
        SharedOptions::add_to(args);

        args.add_parameter(filepath, "--output", "-o")
            .nargs(1).absent("mandelbrot.ppm")
            .help("Path to output file");
        args.add_parameter(scale, "--zoom-scale", "-z")
            .nargs(1).absent(0.4)
            .action(parse_multifloat)
            .help("Zoom scale factor");
        args.add_parameter(max_iterations, "--max-iterations", "-n")
            .nargs(1).absent(0)
            .help("Maximum iterations (0 = auto)");
        args.add_parameter(precision, "--precision", "-p")
            .nargs(1).absent(0)
            .help("Decimal digits (0 = auto)");
        args.add_parameter(numeric_type, "--numeric-type")
            .nargs(1).absent("double")
            .help("Number type: double, long-double, dexp");
    }
};

struct DirectOptions : argumentum::CommandOptions, public ImageOptions {
    using CommandOptions::CommandOptions;
    void add_parameters(argumentum::ParameterConfig& args) override {
        add_to(args);
    }
};

struct PerturbedOptions : argumentum::CommandOptions, public ImageOptions {
    using CommandOptions::CommandOptions;
    void add_parameters(argumentum::ParameterConfig& args) override {
        add_to(args);
    }
};

struct BLAOptions : argumentum::CommandOptions, public ImageOptions {
    using CommandOptions::CommandOptions;

    std::pair<std::size_t, std::size_t> probe_grid {3, 3};
    double tolerance {1e-8};
    double lower_exp {-(1 << 12)};
    double upper_exp {-(1 << 0)};
    double epsilon {0.0};
    std::size_t first_level {0};

    void add_parameters(argumentum::ParameterConfig& args) override {
        ImageOptions::add_to(args);

        args.add_parameter(probe_grid, "--probes")
            .nargs(1).absent({3, 3})
            .action(parse_probes_pair)
            .help("Probe grid dimensions (rows x cols)");
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

struct VideoOptions : argumentum::CommandOptions, public SharedOptions {
    using CommandOptions::CommandOptions;

    std::string directory {"mandelbrot"};
    MultiFloat initial_scale {"0.4"};
    MultiFloat final_scale {"0.4"};
    double frames_per_second {24.0};
    double zoom_per_second {2.0};

    void add_parameters(argumentum::ParameterConfig& args) override {
        SharedOptions::add_to(args);

        args.add_parameter(directory, "--output", "-o")
            .nargs(1).absent("mandelbrot")
            .help("Path to the directory where video frames will be written to");
        args.add_parameter(initial_scale, "--initial-scale", "-a")
            .nargs(1).absent(0.5)
            .action(parse_multifloat)
            .help("Zoom factor at the first frame");
        args.add_parameter(final_scale, "--final-scale", "-b")
            .nargs(1).absent(128.0)
            .action(parse_multifloat)
            .help("Zoom factor at the last frame");
        args.add_parameter(frames_per_second, "--fps")
            .nargs(1).absent(24.0)
            .help("Frames per second");
        args.add_parameter(zoom_per_second, "--zoom-per-second")
            .nargs(1).absent(2.0)
            .help("Zoom factor applied each second");
    }
};

} // namespace wacfrac
