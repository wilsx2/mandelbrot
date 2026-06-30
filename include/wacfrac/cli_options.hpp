#pragma once

#include "wacfrac/resolution.hpp"
#include "wacfrac/types.hpp"
#include "wacfrac/log.hpp"
#include "wacfrac/constants.hpp"
#include "wacfrac/color.hpp"
#include "wacfrac/viewport.hpp"
#include <argumentum/argparse.h>
#include <argumentum/inc/optionpack.h>
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <ranges>

namespace wacfrac {

template <typename F>
auto make_nargs2_parser(F&& on_complete) {
    return [state = std::make_shared<std::array<std::string, 2>>(), on_complete]
    (auto& target, const std::string& value) {
        if ((*state)[0].empty()) {
            (*state)[0] = value;
        } else {
            (*state)[1] = value;
            on_complete(target, *state);
            state->fill(std::string{});
        }
    };
}

template <typename F>
auto make_nargs3_parser(F&& on_complete) {
    return [state = std::make_shared<std::array<std::string, 3>>(), on_complete]
    (auto& target, const std::string& value) {
        if ((*state)[0].empty()) {
            (*state)[0] = value;
        } else if ((*state)[1].empty()){
            (*state)[1] = value;
        } else {
            (*state)[2] = value;
            on_complete(target, *state);
            state->fill(std::string{});
        }
    };
}

static void parse_multifloat(MultiFloat& target, const std::string& value){
    target = MultiFloat(value, 2000); // NOTE: Magic number
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
            .action(make_nargs2_parser([](auto& target, const std::array<std::string, 2>& parts){
                target.width = std::stoul(parts[0]);
                target.height = std::stoul(parts[1]);
            }))
            .help("Width and height of output image");
        args.add_parameter(focus, "--focus", "-f")
            .nargs(2).absent({-0.5, 0.0})
            .action(make_nargs2_parser([](auto& target, const std::array<std::string, 2>& parts){
                target = MultiComplex{MultiFloat{parts[0], 2000}, MultiFloat{parts[1], 2000}, 2000}; // NOTE: Magic number
            }))
            .help("Coordinates to zoom in on");
        args.add_parameter(escape_radius, "--escape-radius", "-e")
            .nargs(1).absent(2.0)
            .help("Escape radius");
        args.add_parameter(palette, "--color-palette", "-c")
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
        args.add_parameter(continuous_coloring, "--smooth-coloring", "-s")
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
        args.add_parameter(numeric_type, "--numeric-type", "-t")
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
            .nargs(2).absent({3, 3})
            .action(make_nargs2_parser([](auto& target, const std::array<std::string, 2>& parts){
                target.first = std::stoul(parts[0]);
                target.second = std::stoul(parts[1]);
            }))
            .help("Probe grid dimensions (rows cols)");
        args.add_parameter(tolerance, "--tolerance", "-t")
            .nargs(1).absent(1e-8)
            .help("Epsilon search tolerance");
        args.add_parameter(lower_exp, "--lower-exp", "-l")
            .nargs(1).absent(-(1 << 12))
            .help("Lower exponent for epsilon search");
        args.add_parameter(upper_exp, "--upper-exp", "-u")
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
    MultiFloat initial_scale {2.0};
    MultiFloat final_scale {1.25};
    double frames_per_second {24.0};
    double zoom_per_second {2.0};
    std::size_t segment_size {64};
    std::tuple<double, double, double> iteration_parameters {250.0, 50.0, 1.5};
    // std::size_t cutoffs {}; // float direct | double direct | float perturbed | double perturbed | double bla | doubleexp bla

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
        args.add_parameter(zoom_per_second, "--zoom-per-second", "-z")
            .nargs(1).absent(2.0)
            .help("Zoom factor applied each second");
        args.add_parameter(segment_size, "--segment-size", "-S")
            .nargs(1).absent(64)
            .help("Frames in each video segment from which the final video is composed");
        args.add_parameter(iteration_parameters, "--iteration-parameters", "-n")
            .nargs(3).absent({250.0, 50.0, 1.5})
            .action(make_nargs3_parser([](auto& target, const std::array<std::string, 3>& parts){
                std::get<0>(target) = std::stoul(parts[0]);
                std::get<1>(target) = std::stoul(parts[1]);
                std::get<2>(target) = std::stoul(parts[2]);
            }))
            .help("(mod, fact, exp) -> max_n = mod + fact * exponential_scale^exp");
    }
};

} // namespace wacfrac
