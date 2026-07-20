#pragma once

#include "wacfrac/log.hpp"
#include "wacfrac/renderer.hpp"
#include "wacfrac/types.hpp"
#include <argumentum/argparse.h>
#include <array>
#include <boost/optional.hpp>
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <ranges>

namespace wacfrac {

constexpr std::size_t DEFAULT_MP_PRECISION = 2000; // WARN: Dangerous assumption

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
    target = MultiFloat(value, DEFAULT_MP_PRECISION);
}

static void parse_multicomplex(MultiComplex& target, const std::string& value){
    std::string normalized(value);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
        [](unsigned char c) { return std::isspace(c) ? ' ' : c; });
    std::stringstream ss {normalized};
    std::string real, imag;
    std::getline(ss, real, ' ');
    std::getline(ss, imag, ' ');

    target = MultiComplex(real, imag, DEFAULT_MP_PRECISION);
}

template<typename Context>
static void parse_palette_string(typename Context::template Buffer<Pixel>& target, const std::string& value) {
    Context ctx {}; // WARN: Should be an arg
    std::string normalized(value);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
        [](unsigned char c) { return std::isspace(c) ? ' ' : c; });
    auto view {normalized | std::views::split(' ') | std::views::transform([](auto subrange) {
            return parse_color(std::string_view(&*subrange.begin(), subrange.size()));
        }) | std::views::enumerate};
    target = ctx.template make_buffer<Pixel>(std::ranges::distance(view));
    for (auto&& [idx, pixel] : view) {
        target.as_span()[idx] = pixel;
    }
}

template<typename Context>
struct RendererOptions : public RendererConfig<Context> {
    bool prefer_cpu {false};
    unsigned log_level {2};

    void add_parameters(argumentum::ParameterConfig& args) {
        args.add_parameter(prefer_cpu, "--prefer-cpu", "-C")
            .nargs(0).absent(prefer_cpu)
            .help("Use the CPU even when the GPU is available");
        args.add_parameter(this->resolution, "--resolution", "-r")
            .nargs(2).absent(this->resolution)
            .action(make_nargs2_parser([](auto& target, const std::array<std::string, 2>& parts){
                target.width = std::stoul(parts[0]);
                target.height = std::stoul(parts[1]);
            }))
            .help("Width and height of output image");
        args.add_parameter(this->focus, "--focus", "-f")
            .nargs(2).absent(this->focus)
            .action(make_nargs2_parser([](auto& target, const std::array<std::string, 2>& parts){
                target = MultiComplex(parts[0], parts[1], DEFAULT_MP_PRECISION);
            }))
            .help("Coordinates to zoom in on");
        args.add_parameter(this->escape_radius, "--escape-radius", "-e")
            .nargs(1).absent(this->escape_radius)
            .help("Escape radius");
        args.add_parameter(this->palette, "--color-palette", "-c")
            .nargs(1) /*  NOTE: Argumentum requires .absent to take a const&, and without absent 
                       *        will zero initialize the parameter. Buffer cannot be
                       *        copied, so the default value is reinitialized in Renderer.
                       *        I hate argumentum. */
            .action(parse_palette_string<Context>)
            .help("Hex formatted colors mapped to escape time");
        args.add_parameter(this->discrete_coloring, "--discrete-coloring", "-d")
            .nargs(0).absent(this->discrete_coloring)
            .help("Disable smooth/continuous coloring");
        args.add_parameter(this->iteration_parameters, "--iteration-parameters", "-N")
            .nargs(3).absent(this->iteration_parameters)
            .action(make_nargs3_parser([](auto& target, const std::array<std::string, 3>& parts){
                target.modifier = std::stod(parts[0]);
                target.factor = std::stod(parts[1]);
                target.exponent = std::stod(parts[2]);
            }))
            .help("func max_iterations(mod, fact, exp) = mod + fact * exponential_scale^exp");
        args.add_parameter(log_level, "--log-level")
            .nargs(1).absent(log_level)
            .help("Log level: 0=Trace, 1=Debug, 2=Info, 3=Warning, 4=Error, 5=Fatal");

        args.add_parameter(this->probe_grid, "--probes", "-P")
            .nargs(2).absent(this->probe_grid)
            .action(make_nargs2_parser([](auto& target, const std::array<std::string, 2>& parts){
                target.first = std::stoul(parts[0]);
                target.second = std::stoul(parts[1]);
            }))
            .help("Probe grid dimensions (rows cols)");
        args.add_parameter(this->bla_config.tolerance, "--tolerance", "-T")
            .nargs(1).absent(this->bla_config.tolerance)
            .help("Epsilon search tolerance");
        args.add_parameter(this->bla_config.lower_exp, "--lower-exp", "-l")
            .nargs(1).absent(this->bla_config.lower_exp)
            .help("Lower exponent for epsilon search");
        args.add_parameter(this->bla_config.upper_exp, "--upper-exp", "-u")
            .nargs(1).absent(this->bla_config.upper_exp)
            .help("Upper exponent for epsilon search");
        args.add_parameter(this->bla_config.first_level, "--first-level", "-L")
            .nargs(1).absent(this->bla_config.first_level)
            .help("First BLA level (0 = auto)");
    }
};

constexpr double DIRECT_THRESHOLD = 1e13;
constexpr double PERTURB_THRESHOLD = 1e25;

struct ImageOptions : public ImageConfig, public argumentum::CommandOptions {
    using CommandOptions::CommandOptions;

    std::string filepath {"mandelbrot.ppm"};

    void add_parameters(argumentum::ParameterConfig& args) override {
        args.add_parameter(filepath, "--output", "-o")
            .nargs(1).absent(filepath)
            .help("Path to output file");
        args.add_parameter(this->scale, "--zoom-scale", "-z")
            .nargs(1).absent(this->scale)
            .action(parse_multifloat)
            .help("Zoom scale factor");
        args.add_parameter(this->max_iterations, "--max-iterations", "-n")
            .nargs(1).absent(this->max_iterations)
            .help("Maximum iterations (0 = auto)");
        args.add_parameter(this->precision, "--precision", "-p")
            .nargs(1).absent(this->precision)
            .help("Decimal digits (0 = auto)");
        args.add_parameter(this->numeric_type, "--numeric-type", "-t")
            .nargs(1).absent(this->numeric_type)
            .action([](auto& target, const std::string& value){
                target = [&](){
                    if (value == "auto") {
                        return NumericType::Auto;
                    } else if (value == "float") {
                        return NumericType::Float;
                    } else if (value == "double") {
                        return NumericType::Double;
                    } else if (value == "dexp") {
                        return NumericType::DoubleExp;
                    }
                    logging::warning("\"{}\" is not a recognized numeric type, falling back to auto", value);
                    return NumericType::Auto;
                }();
            })
            .help("Number type: auto, float, double, dexp");
        args.add_parameter(this->render_type, "--render-type", "-R") // WARN: Above
            .nargs(1).absent(this->render_type)
            .action([](auto& target, const std::string& value){
                target = [&](){
                    if (value == "auto") {
                        return RenderType::Auto;
                    } else if (value == "direct") {
                        return RenderType::Direct;
                    } else if (value == "perturbed") {
                        return RenderType::Perturbed;
                    } else if (value == "bla") {
                        return RenderType::BLA;
                    }
                    logging::warning("\"{}\" is not a recognized numeric type, falling back to auto", value);
                    return RenderType::Auto;
                }();
            })
            .help("Number type: auto, direct, perturbed, bla");
        args.add_parameter(this->epsilon, "--epsilon", "-E")
            .nargs(1).absent(this->epsilon)
            .help("Direct epsilon value (0 = use binary search)");
    }
};

struct VideoConfig {
    double frames_per_second {24.0};
    std::size_t segment_size {64};
    MultiFloat initial_scale {0.4};
    MultiFloat final_scale {1e1};
    double zoom_per_second {2.0};
};

struct VideoOptions : public VideoConfig, public argumentum::CommandOptions {
    using CommandOptions::CommandOptions;

    std::string directory {"mandelbrot"};

    void add_parameters(argumentum::ParameterConfig& args) override {
        args.add_parameter(directory, "--output", "-o")
            .nargs(1).absent(directory)
            .help("Path to the directory where video frames will be written to");
        args.add_parameter(this->initial_scale, "--initial-scale", "-a")
            .nargs(1).absent(this->initial_scale)
            .action(parse_multifloat)
            .help("Zoom factor at the first frame");
        args.add_parameter(this->final_scale, "--final-scale", "-b")
            .nargs(1).absent(this->final_scale)
            .action(parse_multifloat)
            .help("Zoom factor at the last frame");
        args.add_parameter(this->frames_per_second, "--fps")
            .nargs(1).absent(this->frames_per_second)
            .help("Frames per second");
        args.add_parameter(this->zoom_per_second, "--zoom-per-second", "-z")
            .nargs(1).absent(this->zoom_per_second)
            .help("Zoom factor applied each second");
        args.add_parameter(this->segment_size, "--segment-size", "-S")
            .nargs(1).absent(this->segment_size)
            .help("Frames in each video segment from which the final video is composed");
    }
};

} // namespace wacfrac
