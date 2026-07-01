#include "wacfrac/cli_options.hpp"
#include "wacfrac/wacfrac.hpp"
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <limits>
#include <string_view>

namespace {

constexpr double DIRECT_THRESHOLD = 1e13;
constexpr double PERTURB_THRESHOLD = 1e25;

template <typename T>
struct NumericTypeTag { using type = T; };

template <typename F>
decltype(auto) with_numeric_type(const std::string& type, F&& f) {
    if (type == "double")
        return f(NumericTypeTag<std::complex<double>>{});
    if (type == "long-double")
        return f(NumericTypeTag<std::complex<long double>>{});
    if (type == "dexp")
        return f(NumericTypeTag<wacfrac::DoubleExpComplex>{});
    wacfrac::logging::error("Unknown numeric type: {}", type);
    std::exit(EXIT_FAILURE);
}

auto make_viewport(const wacfrac::MultiComplex& center,
                   const wacfrac::MultiFloat& scale,
                   const wacfrac::Resolution& res) {
    wacfrac::Viewport view;
    view.center = center;
    auto aspect_ratio = res.width / static_cast<double>(res.height);
    if (aspect_ratio >= 1.0) {
        view.dimensions = {aspect_ratio, 1.0};
    } else {
        view.dimensions = {1.0, 1.0 / aspect_ratio};
    }
    return view.zoomed(scale);
}

void log_render_config(std::string_view label,
                       const wacfrac::Resolution& res,
                       const wacfrac::MultiComplex& focus,
                       const wacfrac::MultiFloat& scale,
                       std::size_t max_iterations,
                       std::size_t precision,
                       std::string_view numeric_type) {
    wacfrac::logging::info(
        "{}: resolution={}x{} focus=({}, {}) zoom={} max_iterations={} precision={} numeric_type={}",
        label, res.width, res.height,
        focus.real(), focus.imag(), scale,
        max_iterations, precision, numeric_type);
}

auto make_view(wacfrac::ImageOptions& opt) {
    auto view = make_viewport(opt.focus, opt.scale, opt.resolution);
    opt.precision = opt.precision ? opt.precision : view.required_precision();

    wacfrac::MultiFloat::default_precision(opt.precision);
    wacfrac::MultiComplex::default_precision(opt.precision);
    view.precision(opt.precision);

    log_render_config("Configuration", opt.resolution, opt.focus, opt.scale,
                      opt.effective_max_iterations(), opt.precision,
                      opt.effective_numeric_type());
    wacfrac::logging::info("Configuration: output={}", opt.filepath);

    return view;
}

auto make_color_fn(const wacfrac::SharedOptions& opts, std::size_t max_iterations)
    -> std::function<wacfrac::Pixel(std::complex<float>, std::size_t)>
{
    const auto& pal = opts.palette.empty() ? wacfrac::palette::ULTRA : opts.palette;
    if (opts.discrete_coloring) {
        return [&pal, max_iterations](std::complex<float>, std::size_t n) {
            return wacfrac::colorize_discrete(pal, max_iterations, n);
        };
    }
    return [&pal, max_iterations](std::complex<float> z, std::size_t n) {
        return wacfrac::colorize_continuous(pal, max_iterations, z, n);
    };
}

template <typename T>
void direct_render_pass(std::span<wacfrac::Pixel> pixels,
                        const wacfrac::Resolution& res,
                        const wacfrac::Viewport& view,
                        const wacfrac::SharedOptions& opts,
                        std::size_t max_iterations) {
    wacfrac::absolute_render<T>(pixels, res, view,
        [max_iterations, &opts](T c) {
            return wacfrac::escape<T>(c, max_iterations, opts.escape_radius);
        },
        make_color_fn(opts, max_iterations));
}

template <typename T>
void perturbed_render_pass(std::span<wacfrac::Pixel> pixels,
                           const wacfrac::Resolution& res,
                           const wacfrac::Viewport& view,
                           const wacfrac::SharedOptions& opts,
                           std::size_t max_iterations,
                           const std::vector<T>& ref,
                           const wacfrac::MultiComplex& c_ref) {
    wacfrac::perturbed_render<T>(pixels, res, view,
        [&ref, max_iterations, &opts](T dc) {
            return wacfrac::escape_perturbed<T>(ref, dc, max_iterations, opts.escape_radius);
        },
        make_color_fn(opts, max_iterations),
        c_ref);
}

template <typename T>
void bla_render_pass(std::span<wacfrac::Pixel> pixels,
                     const wacfrac::Resolution& res,
                     const wacfrac::Viewport& view,
                     const wacfrac::SharedOptions& opts,
                     std::size_t max_iterations,
                     const wacfrac::BivariateLinearApproximator<T>& bla,
                     const wacfrac::MultiComplex& c_ref) {
    auto total_skipped = std::atomic<std::uint64_t>{0};
    wacfrac::perturbed_render<T>(pixels, res, view,
        [&bla, &total_skipped](T dc) {
            auto result = bla.escape_approximate(dc);
            total_skipped += std::get<2>(result);
            return result;
        },
        make_color_fn(opts, max_iterations),
        c_ref);
    auto avg_skipped = static_cast<double>(total_skipped) / res.area();
    wacfrac::logging::info("BLA render complete (avg skipped: {})", avg_skipped);
}

template <typename F>
void render_image(wacfrac::ImageOptions& opts, F&& render_fn) {
    std::vector<wacfrac::Pixel> pixels(opts.resolution.area());

    auto t_render = std::chrono::steady_clock::now();

    auto view = make_view(opts);
    with_numeric_type(opts.effective_numeric_type(), [&](auto tag) {
        render_fn(view, pixels, tag);
    });

    auto render_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t_render);

    wacfrac::logging::info("Render took {}ms", render_ms.count());
    wacfrac::write_ppm(opts.filepath, opts.resolution, pixels);
}

enum class RenderMode { Direct, Perturbed, BLA };

template <RenderMode Mode>
void render_with_mode(wacfrac::ImageOptions& opts,
                      const wacfrac::BLASearchOptions& bla_opts = {},
                      double bla_epsilon = 0.0) {
    auto max_iterations = opts.effective_max_iterations();
    render_image(opts, [&, max_iterations]<typename T>(const auto& view,
                                                       auto& pixels,
                                                       NumericTypeTag<T>) {
        if constexpr (Mode == RenderMode::Direct) {
            direct_render_pass<T>(pixels, opts.resolution, view, opts,
                                  max_iterations);
        } else {
            auto c_ref = view.center;
            auto ref = wacfrac::compute_reference_mt<T>(
                c_ref, max_iterations, std::numeric_limits<double>::infinity());
            wacfrac::logging::info(
                "Reference at view center with orbit length {}", ref.size());

            if constexpr (Mode == RenderMode::Perturbed) {
                perturbed_render_pass<T>(pixels, opts.resolution, view, opts,
                                         max_iterations, ref, c_ref);
            } else {
                using CT = wacfrac::ComplexValueTypeT<T>;
                auto last_level = static_cast<std::size_t>(std::log2(ref.size()));
                auto first_level = bla_opts.first_level != 0
                    ? bla_opts.first_level
                    : std::max(0uz, last_level > 9 ? last_level - 9 : 0uz);
                auto probes = view.template generate_probes<T>(
                    bla_opts.probe_grid.first, bla_opts.probe_grid.second);
                auto max_dc = wacfrac::to_complex<T>(view.compute_max_dc(c_ref));
                wacfrac::BivariateLinearApproximator<T> bla{
                    bla_epsilon != 0.0
                        ? wacfrac::BivariateLinearApproximator<T>{
                            static_cast<CT>(bla_epsilon), max_dc, ref,
                            first_level, opts.escape_radius}
                        : wacfrac::BivariateLinearApproximator<T>{
                            bla_opts.lower_exp, bla_opts.upper_exp,
                            bla_opts.tolerance, probes, max_dc, ref,
                            first_level, opts.escape_radius}
                };
                bla_render_pass<T>(pixels, opts.resolution, view, opts,
                                   max_iterations, bla, c_ref);
            }
        }
    });
}

void render_automatic(wacfrac::AutomaticOptions& opts) {
    if (opts.scale > PERTURB_THRESHOLD) {
        render_with_mode<RenderMode::BLA>(opts);
    } else if (opts.scale > DIRECT_THRESHOLD) {
        render_with_mode<RenderMode::Perturbed>(opts);
    } else {
        render_with_mode<RenderMode::Direct>(opts);
    }
}

template <typename T>
auto select_ref(const wacfrac::ReferenceSet& refs) -> const std::vector<T>& {
    if constexpr (std::is_same_v<T, std::complex<double>>)
        return refs.double_ref;
    else if constexpr (std::is_same_v<T, std::complex<long double>>)
        return refs.long_double_ref;
    else
        return refs.dexp_ref;
}

void render_video_frame(std::span<wacfrac::Pixel> pixels,
                        const wacfrac::VideoOptions& opts,
                        const wacfrac::MultiFloat& scale,
                        const wacfrac::ReferenceSet& refs,
                        const wacfrac::MultiComplex& c_ref) {
    auto view = make_viewport(opts.focus, scale, opts.resolution);
    auto frame_precision = view.required_precision();
    auto frame_max_iter = view.required_iterations(
        std::get<0>(opts.iteration_parameters),
        std::get<1>(opts.iteration_parameters),
        std::get<2>(opts.iteration_parameters));

    wacfrac::MultiFloat::default_precision(frame_precision);
    wacfrac::MultiComplex::default_precision(frame_precision);
    view.precision(frame_precision);

    auto scale_d = static_cast<double>(scale);

    std::string frame_type = "double";
    if (frame_precision > 1000)
        frame_type = "dexp";
    else if (frame_precision > 230)
        frame_type = "long-double";

    log_render_config("Frame", opts.resolution, opts.focus, scale,
                      frame_max_iter, frame_precision, frame_type);

    if (scale_d <= DIRECT_THRESHOLD) {
        wacfrac::logging::info("Frame: algorithm=direct");
        with_numeric_type(frame_type, [&](auto tag) {
            using T = typename decltype(tag)::type;
            direct_render_pass<T>(pixels, opts.resolution, view, opts,
                                  frame_max_iter);
        });
    } else {
        with_numeric_type(frame_type, [&](auto tag) {
            using T = typename decltype(tag)::type;
            const auto& ref = select_ref<T>(refs);

            if (scale_d <= PERTURB_THRESHOLD) {
                wacfrac::logging::info("Frame: algorithm=perturbed");
                perturbed_render_pass<T>(pixels, opts.resolution, view, opts,
                                         frame_max_iter, ref, c_ref);
            } else {
                wacfrac::logging::info("Frame: algorithm=BLA");
                auto probes = view.template generate_probes<T>(
                    opts.probe_grid.first, opts.probe_grid.second);
                auto max_dc = wacfrac::to_complex<T>(
                    view.compute_max_dc(c_ref));
                wacfrac::BivariateLinearApproximator<T> bla{
                    opts.upper_exp, opts.lower_exp,
                    opts.tolerance, probes, max_dc, ref, opts.first_level,
                    opts.escape_radius
                };
                bla_render_pass<T>(pixels, opts.resolution, view, opts,
                                   frame_max_iter, bla, c_ref);
            }
        });
    }
}

void render_video(wacfrac::VideoOptions& opts) {
    auto final_view = make_viewport(opts.focus, opts.final_scale, opts.resolution);
    auto max_iterations = final_view.required_iterations();
    auto precision = final_view.required_precision();

    wacfrac::MultiFloat::default_precision(precision);
    wacfrac::MultiComplex::default_precision(precision);

    auto refs = wacfrac::compute_references_all(
        opts.focus, max_iterations, std::numeric_limits<double>::infinity());

    wacfrac::logging::info(
        "Video pre-compute: final_zoom={} max_iterations={} precision={} ref_size={}",
        opts.final_scale, max_iterations, precision, refs.double_ref.size());

    wacfrac::write_zoom_frames(
        opts.directory, opts.segment_size, opts.resolution,
        opts.initial_scale, opts.final_scale,
        opts.zoom_per_second, opts.frames_per_second,
        [&](std::span<wacfrac::Pixel> pixels, wacfrac::MultiFloat scale) {
            render_video_frame(pixels, opts, scale, refs, opts.focus);
        });
}

void dispatch_render(argumentum::CommandOptions* cmd) {
    using namespace wacfrac;

    if (auto* p = dynamic_cast<DirectOptions*>(cmd))
        return render_with_mode<RenderMode::Direct>(*p);
    if (auto* p = dynamic_cast<PerturbedOptions*>(cmd))
        return render_with_mode<RenderMode::Perturbed>(*p);
    if (auto* p = dynamic_cast<BLAOptions*>(cmd))
        return render_with_mode<RenderMode::BLA>(*p, *p, p->epsilon);
    if (auto* p = dynamic_cast<AutomaticOptions*>(cmd))
        return render_automatic(*p);
    if (auto* p = dynamic_cast<VideoOptions*>(cmd))
        return render_video(*p);
}

} // namespace

int main(int argc, char* argv[])
{
    auto parser = argumentum::argument_parser{};
    auto params = parser.params();
    parser.config().program(argv[0]).description("Mandelbrot Set Plotter");

    params.add_command<wacfrac::DirectOptions>("direct").help("Direct escape-time rendering");
    params.add_command<wacfrac::PerturbedOptions>("perturbed").help("Perturbation-theory rendering");
    params.add_command<wacfrac::BLAOptions>("bla").help("Bivariate linear approximation rendering");
    params.add_command<wacfrac::AutomaticOptions>("auto").help("Automatically selects the render mode heuristically");
    params.add_command<wacfrac::VideoOptions>("video").help("Sequential rendering (various methods)");

    auto parse_result = parser.parse_args(argc, argv);
    if (!parse_result)
        return EXIT_FAILURE;

    std::shared_ptr<argumentum::CommandOptions> cmd;
    for (auto& pcmd : parse_result.commands)
        if (pcmd) { cmd = pcmd; break; }
    if (!cmd) {
        wacfrac::logging::error(
            "No render mode specified. Use one of: direct, perturbed, bla, video");
        return EXIT_FAILURE;
    }

    if (auto* p = dynamic_cast<wacfrac::SharedOptions*>(cmd.get())) {
        wacfrac::logging::init(p->log_level);
    }
    wacfrac::logging::info("Mandelbrot Set Plotter");
    dispatch_render(cmd.get());
}
