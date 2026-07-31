#include "wacfrac/cli_options.hpp"

#include "wacfrac/color.hpp"

#include <algorithm>
#include <argparse/argparse.hpp>
#include <boost/regex.hpp>
#include <cctype>
#include <iostream>
#include <ranges>

namespace wacfrac
{
namespace
{

constexpr std::size_t DEFAULT_MP_PRECISION = 2000;

auto parse_multifloat(const std::string& value) -> MultiFloat
{
    return MultiFloat(value, DEFAULT_MP_PRECISION);
}

void parse_palette_string(DeviceBuffer<Pixel>& target, const std::string& value)
{
    std::string normalized(value);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
        return std::isspace(c) ? ' ' : c;
    });
    auto view{normalized | std::views::split(' ') | std::views::transform([](auto subrange) {
                  return parse_color(std::string_view(&*subrange.begin(), subrange.size()));
              }) |
              std::views::enumerate};
    target = {target.queue(), static_cast<std::size_t>(std::ranges::distance(view))};
    for (auto&& [idx, pixel] : view) {
        target[idx] = pixel;
    }
}

auto parse_numeric_type(const std::string& value) -> NumericType
{
    if (value == "auto")
        return NumericType::Auto;
    if (value == "float")
        return NumericType::Float;
    if (value == "double")
        return NumericType::Double;
    if (value == "dexp")
        return NumericType::DoubleExp;
    logging::warning("\"{}\" is not a recognized numeric type, falling back to auto", value);
    return NumericType::Auto;
}

auto parse_render_type(const std::string& value) -> RenderType
{
    if (value == "auto")
        return RenderType::Auto;
    if (value == "direct")
        return RenderType::Direct;
    if (value == "perturbed")
        return RenderType::Perturbed;
    if (value == "bla")
        return RenderType::BLA;
    logging::warning("\"{}\" is not a recognized render type, falling back to auto", value);
    return RenderType::Auto;
}

} // anonymous namespace

auto parse_arguments(int argc, char* argv[]) -> std::optional<CliOptions>
{
    argparse::ArgumentParser program("wacfrac", "1.2.0");

    CliOptions opts;
    std::vector<std::string> resolution_strs;
    std::vector<std::string> focus_strs;
    std::vector<std::string> iteration_params_strs;
    std::vector<std::string> probes_strs;

    argparse::ArgumentParser image_cmd("image");
    argparse::ArgumentParser video_cmd("video");

    // Shared params
    auto add_renderer_params = [&](argparse::ArgumentParser& p) {
        auto& r = opts.renderer;
        p.add_argument("--log-level")
            .scan<'u', unsigned>()
            .store_into(opts.log_level)
            .help("Log level: 0=Trace, 1=Debug, 2=Info, 3=Warning, 4=Error, 5=Fatal");
        p.add_argument("--resolution", "-r")
            .nargs(2)
            .store_into(resolution_strs)
            .help("Width and height of output image");
        p.add_argument("--focus", "-f").nargs(2).store_into(focus_strs).help("Coordinates to zoom in on");
        p.add_argument("--escape-radius", "-e")
            .scan<'g', double>()
            .default_value(r.escape_radius)
            .store_into(r.escape_radius)
            .help("Escape radius");
        p.add_argument("--color-palette", "-c")
            .action([&](const std::string& v) {
                parse_palette_string(r.palette, v);
                return v;
            })
            .help("Hex formatted colors mapped to escape time");
        p.add_argument("--discrete-coloring", "-d")
            .flag()
            .store_into(r.discrete_coloring)
            .help("Disable smooth/continuous coloring");
        p.add_argument("--iteration-parameters", "-N")
            .nargs(3)
            .store_into(iteration_params_strs)
            .help("func max_iterations(mod, fact, exp) = mod + fact * exponential_scale^exp");
        p.add_argument("--probes", "-P").nargs(2).store_into(probes_strs).help("Probe grid dimensions (rows cols)");
        p.add_argument("--tolerance", "-T")
            .scan<'g', double>()
            .default_value(r.bla_config.tolerance)
            .store_into(r.bla_config.tolerance)
            .help("Epsilon search tolerance");
        p.add_argument("--lower-exp", "-l")
            .scan<'g', double>()
            .default_value(r.bla_config.lower_exp)
            .store_into(r.bla_config.lower_exp)
            .help("Lower exponent for epsilon search");
        p.add_argument("--upper-exp", "-u")
            .scan<'g', double>()
            .default_value(r.bla_config.upper_exp)
            .store_into(r.bla_config.upper_exp)
            .help("Upper exponent for epsilon search");
        p.add_argument("--first-level", "-L")
            .action([](const std::string& v) {
                return static_cast<std::size_t>(std::stoul(v));
            })
            .default_value(r.bla_config.first_level)
            .help("First BLA level (0 = auto)");
        p.add_argument("--significant-iterations", "-I")
            .default_value(r.bla_threshold)
            .store_into(r.bla_threshold)
            .help("The iteration count after which BLA rendering is used.");
        p.add_argument("--underflow-radius", "-U")
            .default_value(r.underflow_radius)
            .store_into(r.underflow_radius)
            .help("Proportionate to how cautious the renderer will be about underflowing fixed-precision numbers");
    };
    add_renderer_params(image_cmd);
    add_renderer_params(video_cmd);

    // Image params
    image_cmd.add_argument("--output", "-o").store_into(opts.image.filepath).help("Path to output file");
    image_cmd.add_argument("--zoom-scale", "-z")
        .action(parse_multifloat)
        .default_value(opts.image.scale)
        .help("Zoom scale factor");
    image_cmd.add_argument("--max-iterations", "-n")
        .scan<'u', unsigned>()
        .store_into(opts.image.max_iterations)
        .help("Maximum iterations (0 = auto)");
    image_cmd.add_argument("--precision", "-p")
        .action([](const std::string& v) {
            return static_cast<std::size_t>(std::stoul(v));
        })
        .default_value(opts.image.precision)
        .help("Decimal digits (0 = auto)");
    image_cmd.add_argument("--numeric-type", "-t")
        .action(parse_numeric_type)
        .default_value(NumericType::Auto)
        .help("Number type: auto, float, double, dexp");
    image_cmd.add_argument("--render-type", "-R")
        .action(parse_render_type)
        .default_value(RenderType::Auto)
        .help("Render type: auto, direct, perturbed, bla");
    image_cmd.add_argument("--epsilon", "-E")
        .scan<'g', double>()
        .store_into(opts.image.epsilon)
        .help("Direct epsilon value (0 = use binary search)");

    // Video-specific
    video_cmd.add_argument("--output", "-o")
        .store_into(opts.video.directory)
        .help("Path to the directory where video frames will be written to");
    video_cmd.add_argument("--initial-scale", "-a")
        .action(parse_multifloat)
        .default_value(opts.video.initial_scale)
        .help("Zoom factor at the first frame");
    video_cmd.add_argument("--final-scale", "-b")
        .action(parse_multifloat)
        .default_value(opts.video.final_scale)
        .help("Zoom factor at the last frame");
    video_cmd.add_argument("--fps")
        .scan<'g', double>()
        .store_into(opts.video.frames_per_second)
        .help("Frames per second");
    video_cmd.add_argument("--zoom-per-second", "-z")
        .scan<'g', double>()
        .store_into(opts.video.zoom_per_second)
        .help("Zoom factor applied each second");
    video_cmd.add_argument("--segment-size", "-S")
        .action([](const std::string& v) {
            return static_cast<std::size_t>(std::stoul(v));
        })
        .default_value(opts.video.segment_size)
        .help("Frames in each video segment");
    video_cmd.add_argument("--preview", "-p")
        .flag()
        .store_into(opts.video.do_preview)
        .help("Mirror most recent frame to */preview.ppm");

    program.add_subparser(image_cmd);
    program.add_subparser(video_cmd);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        logging::fatal("{}", err.what());
        logging::fatal("{}", program);
        return std::nullopt;
    }

    if (!resolution_strs.empty())
        opts.renderer.resolution = {std::stoul(resolution_strs[0]), std::stoul(resolution_strs[1])};
    if (!focus_strs.empty())
        opts.renderer.focus = MultiComplex(focus_strs[0], focus_strs[1], DEFAULT_MP_PRECISION);
    if (!iteration_params_strs.empty())
        opts.renderer.iteration_parameters = {std::stod(iteration_params_strs[0]), std::stod(iteration_params_strs[1]),
                                              std::stod(iteration_params_strs[2])};
    if (!probes_strs.empty())
        opts.renderer.probe_grid = {std::stoul(probes_strs[0]), std::stoul(probes_strs[1])};

    if (program.is_subcommand_used(image_cmd)) {
        opts.mode = CliOptions::Mode::Image;
        opts.image.numeric_type = image_cmd.get<NumericType>("--numeric-type");
        opts.image.render_type = image_cmd.get<RenderType>("--render-type");
        opts.image.precision = image_cmd.get<std::size_t>("--precision");
        opts.renderer.bla_config.first_level = image_cmd.get<std::size_t>("--first-level");
        opts.image.scale = image_cmd.get<MultiFloat>("--zoom-scale");
    } else if (program.is_subcommand_used(video_cmd)) {
        opts.mode = CliOptions::Mode::Video;
        opts.renderer.bla_config.first_level = video_cmd.get<std::size_t>("--first-level");
        opts.video.segment_size = video_cmd.get<std::size_t>("--segment-size");
        opts.video.initial_scale = video_cmd.get<MultiFloat>("--initial-scale");
        opts.video.final_scale = video_cmd.get<MultiFloat>("--final-scale");
    } else {
        logging::fatal("{}", "No render mode specified. Use either image or video");
        logging::fatal("{}", program);
        return std::nullopt;
    }

    return opts;
}

} // namespace wacfrac
