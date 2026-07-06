#pragma once

#include "wacfrac/log.hpp"
#include "wacfrac/orbit.hpp"
#include "wacfrac/wacfrac.hpp"
#include <argumentum/argparse.h>
#include <array>
#include <boost/optional.hpp>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <ranges>

namespace wacfrac {

constexpr std::size_t DEFAULT_MP_PRECISION = 2000;

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

static void parse_palette_string(std::vector<Pixel>& target, const std::string& value) {
    std::ranges::copy(
        value | std::views::split(' ') | std::views::transform([](auto subrange) {
            return parse_color(std::string_view(&*subrange.begin(), subrange.size()));
        }),
        std::back_inserter(target)
    );
    if (target.empty()) {
        logging::info("Falling back to default palette");
        target = wacfrac::ULTRA;
    }
}

struct SharedOptions {
    Resolution resolution {500, 500};
    MultiComplex focus {-0.5, 0.0};
    double escape_radius {4.0};
    std::vector<Pixel> palette {};
    bool discrete_coloring {false};
    int log_level {2};

    // Iteration
    std::tuple<double, double, double> iteration_parameters {250.0, 50.0, 1.5};

    // BLA Search
    std::pair<std::size_t, std::size_t> probe_grid {8, 8};
    double tolerance {1e-6};
    double lower_exp {-(1 << 12)};
    double upper_exp {-(1 << 6)};
    std::size_t first_level {0};

    void add_parameters(argumentum::ParameterConfig& args) {
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
                target = MultiComplex{MultiFloat{parts[0], DEFAULT_MP_PRECISION}, MultiFloat{parts[1], DEFAULT_MP_PRECISION}, DEFAULT_MP_PRECISION};
            }))
            .help("Coordinates to zoom in on");
        args.add_parameter(escape_radius, "--escape-radius", "-e")
            .nargs(1).absent(4.0)
            .help("Escape radius");
        args.add_parameter(palette, "--color-palette", "-c")
            .minargs(0).absent(ULTRA)
            .action(parse_palette_string)
            .help("Hex formatted colors mapped to escape time");
        args.add_parameter(discrete_coloring, "--discrete-coloring", "-d")
            .nargs(0).absent(false)
            .help("Disable smooth/continuous coloring");
        args.add_parameter(iteration_parameters, "--iteration-parameters", "-N")
            .nargs(3).absent({250.0, 50.0, 1.5})
            .action(make_nargs3_parser([](auto& target, const std::array<std::string, 3>& parts){
                std::get<0>(target) = std::stod(parts[0]);
                std::get<1>(target) = std::stod(parts[1]);
                std::get<2>(target) = std::stod(parts[2]);
            }))
            .help("func max_iterations(mod, fact, exp) = mod + fact * exponential_scale^exp");
        args.add_parameter(log_level, "--log-level")
            .nargs(1).absent(2)
            .help("Log level: 0=Trace, 1=Debug, 2=Info, 3=Warning, 4=Error, 5=Fatal");
        
        // BLA Search
        args.add_parameter(probe_grid, "--probes")
            .nargs(2).absent({8, 8})
            .action(make_nargs2_parser([](auto& target, const std::array<std::string, 2>& parts){
                target.first = std::stoul(parts[0]);
                target.second = std::stoul(parts[1]);
            }))
            .help("Probe grid dimensions (rows cols)");
        args.add_parameter(tolerance, "--tolerance", "-T")
            .nargs(1).absent(1e-6)
            .help("Epsilon search tolerance");
        args.add_parameter(lower_exp, "--lower-exp", "-l")
            .nargs(1).absent(-(1 << 12))
            .help("Lower exponent for epsilon search");
        args.add_parameter(upper_exp, "--upper-exp", "-u")
            .nargs(1).absent(-(1 << 6))
            .help("Upper exponent for epsilon search");
        args.add_parameter(first_level, "--first-level", "-L")
            .nargs(1).absent(0)
            .help("First BLA level (0 = auto)");
    }
};

constexpr double DIRECT_THRESHOLD = 1e13;
constexpr double PERTURB_THRESHOLD = 1e25;
enum class RenderType { Direct , Perturbed , BLA };

struct ImageOptions : argumentum::CommandOptions {
    using CommandOptions::CommandOptions;
    std::shared_ptr<SharedOptions> shared;
    boost::optional<const ReferenceSet&> ref_set;

    std::string filepath {"mandelbrot.ppm"};
    MultiFloat scale {0.4};
    std::size_t max_iterations {0};
    std::size_t precision {0};
    std::string numeric_type {"auto"};
    std::string render_type {"auto"};
    
    // Manual BLA Override
    double epsilon {0.0};

    auto effective_max_iterations() const -> std::size_t {
        return max_iterations ? max_iterations
        : wacfrac::required_iterations(1.0 / scale, 
            std::get<0>(shared->iteration_parameters),
            std::get<1>(shared->iteration_parameters),
            std::get<2>(shared->iteration_parameters)
        );
    }

    auto effective_numeric_type() const -> std::string {
        if (numeric_type != "auto")
            return numeric_type;
        auto p {effective_precision()};
        if (p > 1000) return "dexp";
        if (p > 230)  return "long-double";
        return "double";
    }

    auto effective_precision() const -> std::size_t {
        return precision ? precision : wacfrac::required_precision(1.0 / scale);
    }

    auto effective_render_type() const -> RenderType {
        if (render_type == "auto") {
            if (scale < DIRECT_THRESHOLD) {
                return RenderType::Direct;
            } else if (scale < PERTURB_THRESHOLD) {
                return RenderType::Perturbed;
            } else {
                return RenderType::BLA;
            }
        } else if (render_type == "direct")
            return RenderType::Direct;
        else if (render_type == "perturbed")
            return RenderType::Perturbed;
        else if (render_type == "bla")
            return RenderType::BLA;
        wacfrac::logging::warning("Invalid render type '{}' provided, falling back to direct", render_type);
        return RenderType::Direct;
    }

    void add_parameters(argumentum::ParameterConfig& args) override {
        shared = std::make_shared<SharedOptions>();
        shared->add_parameters(args);

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
            .nargs(1).absent("auto")
            .help("Number type: auto, double, long-double, dexp");
        args.add_parameter(render_type, "--render-type", "-R")
            .nargs(1).absent("auto")
            .help("Number type: auto, direct, perturbed, bla");
        args.add_parameter(epsilon, "--epsilon", "-E")
            .nargs(1).absent(0.0)
            .help("Direct epsilon value (0 = use binary search)");
    }
};

struct VideoOptions : argumentum::CommandOptions {
    using CommandOptions::CommandOptions;
    std::shared_ptr<SharedOptions> shared;

    // Files
    std::string directory {"mandelbrot"};
    double frames_per_second {24.0};
    std::size_t segment_size {64};
    
    // Zoom
    MultiFloat initial_scale {0.4};
    MultiFloat final_scale {1e1};
    double zoom_per_second {2.0};

    void add_parameters(argumentum::ParameterConfig& args) override {
        shared = std::make_shared<SharedOptions>();
        shared->add_parameters(args);

        args.add_parameter(directory, "--output", "-o")
            .nargs(1).absent("mandelbrot")
            .help("Path to the directory where video frames will be written to");
        args.add_parameter(initial_scale, "--initial-scale", "-a")
            .nargs(1).absent(0.4)
            .action(parse_multifloat)
            .help("Zoom factor at the first frame");
        args.add_parameter(final_scale, "--final-scale", "-b")
            .nargs(1).absent(1e1)
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
    }
};

} // namespace wacfrac
