#include "wacfrac/cli_options.hpp"
#include "wacfrac/color.hpp"
#include "wacfrac/constants.hpp"
#include "wacfrac/io.hpp"
#include "wacfrac/types.hpp"
#include "wacfrac/viewport.hpp"
#include "wacfrac/wacfrac.hpp"
#include <atomic>
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <limits>
#include <ratio>
#include <string>
#include <string_view>

namespace {

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
    wacfrac::logging::error(
        "Unknown numeric type: {}", type);
    std::exit(EXIT_FAILURE);
}

auto get_max_iterations(const wacfrac::ImageOptions& opt) {
    return opt.max_iterations ? opt.max_iterations : wacfrac::required_iterations(1.0/opt.scale);
}

auto make_viewport(const wacfrac::MultiComplex& center, const wacfrac::MultiFloat& scale, const wacfrac::Resolution& res) {
    wacfrac::Viewport view;
    view.center = center;
    auto aspect_ratio {res.width / static_cast<double>(res.height)};
    if (aspect_ratio >= 1.0) {
        view.dimensions = {aspect_ratio, 1.0};
    } else {
        view.dimensions = {1.0, 1.0/aspect_ratio};
    }
    return view.zoomed(scale);
}

auto make_view(wacfrac::ImageOptions& opt) {
    auto view = make_viewport(opt.focus, opt.scale, opt.resolution);
    auto precision = opt.precision ? opt.precision : view.required_precision();

    wacfrac::MultiFloat::default_precision(precision);
    wacfrac::MultiComplex::default_precision(precision);
    view.precision(precision);

    wacfrac::logging::info(
        "Configuration: output={} resolution={}x{} focus=({}, {}) zoom={} max_iterations={} precision={} numeric_type={}",
        opt.filepath, opt.resolution.width, opt.resolution.height,
        opt.focus.real(), opt.focus.imag(), opt.scale,
        get_max_iterations(opt), opt.precision, opt.numeric_type);

    return view;
}

struct RenderConfig {
    std::size_t max_iterations;
    double escape_radius;
    const std::vector<wacfrac::Pixel>* palette;
    bool continuous_coloring;
};

auto make_color_fn(const std::vector<wacfrac::Pixel>& palette, bool continuous, std::size_t max_iterations)
    -> std::function<wacfrac::Pixel(std::complex<float>, std::size_t)>
{
    if (continuous) {
        return std::bind_front(wacfrac::colorize_continuous, palette, max_iterations);
    }
    return [max_iterations, &palette](std::complex<float> /*z*/, std::size_t n) -> wacfrac::Pixel {
        return wacfrac::colorize_discrete(palette, max_iterations, n);
    };
}

template <typename T>
void direct_render_pass(std::span<wacfrac::Pixel> pixels,
                        const wacfrac::Resolution& res, const wacfrac::Viewport& view,
                        const RenderConfig& cfg) {
    wacfrac::absolute_render<T>(pixels, res, view,
        [max_iterations = cfg.max_iterations, escape_radius = cfg.escape_radius](T c) {
            return wacfrac::escape<T>(c, max_iterations, escape_radius);
        },
        make_color_fn(*cfg.palette, cfg.continuous_coloring, cfg.max_iterations));
}

template <typename T>
void perturbed_render_pass(std::span<wacfrac::Pixel> pixels,
                           const wacfrac::Resolution& res, const wacfrac::Viewport& view,
                           const RenderConfig& cfg,
                           const std::vector<T>& ref, const wacfrac::MultiComplex& c_ref) {
    wacfrac::perturbed_render<T>(pixels, res, view,
        [&ref, max_iterations = cfg.max_iterations, escape_radius = cfg.escape_radius](T dc) {
            return wacfrac::escape_perturbed<T>(ref, dc, max_iterations, escape_radius);
        },
        make_color_fn(*cfg.palette, cfg.continuous_coloring, cfg.max_iterations),
        c_ref);
}

template <typename T>
void bla_render_pass(std::span<wacfrac::Pixel> pixels,
                     const wacfrac::Resolution& res, const wacfrac::Viewport& view,
                     const RenderConfig& cfg,
                     const wacfrac::BivariateLinearApproximator<T>& bla,
                     const wacfrac::MultiComplex& c_ref) {
    auto total_skipped = std::atomic<std::uint64_t>{0};
    wacfrac::perturbed_render<T>(pixels, res, view,
        [&bla, &total_skipped](T dc) {
            auto result = bla.escape_approximate(dc);
            total_skipped += std::get<2>(result);
            return result;
        },
        make_color_fn(*cfg.palette, cfg.continuous_coloring, cfg.max_iterations),
        c_ref);
    auto avg_skipped = static_cast<double>(total_skipped) / res.area();
    wacfrac::logging::info(
        "BLA render complete (avg skipped: {})", avg_skipped);
}

} // anonymous namespace

template <typename F>
static void render_image(wacfrac::ImageOptions& opts, F&& render_fn) {
    std::vector<wacfrac::Pixel> pixels (opts.resolution.area());

    auto t_render {std::chrono::steady_clock::now()};

    auto view {make_view(opts)};
    with_numeric_type(opts.numeric_type, [&](auto tag) {
        render_fn(view, pixels, tag);
    });

    auto render_ms {std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t_render
    )};

    wacfrac::logging::info( "Render took {}ms", render_ms.count());
    wacfrac::write_ppm(opts.filepath, opts.resolution, pixels);
}

static void render_direct(wacfrac::DirectOptions& opts) {
    RenderConfig cfg{get_max_iterations(opts), opts.escape_radius, &opts.palette, opts.continuous_coloring};
    render_image(opts, [&, cfg]<typename T>(const auto& view, auto& pixels, NumericTypeTag<T>){
        direct_render_pass<T>(pixels, opts.resolution, view, cfg);
    });
}
static void render_perturbed(wacfrac::PerturbedOptions& opts) {
    RenderConfig cfg{get_max_iterations(opts), opts.escape_radius, &opts.palette, opts.continuous_coloring};
    render_image(opts, [&, cfg]<typename T>(const auto& view, auto& pixels, NumericTypeTag<T>){
        auto c_ref = view.center;
        auto ref = wacfrac::compute_reference_mt<T>(c_ref, get_max_iterations(opts), std::numeric_limits<double>::infinity());
        wacfrac::logging::info(
            "Reference at view center with orbit length {}", ref.size());
        perturbed_render_pass<T>(pixels, opts.resolution, view, cfg, ref, c_ref);
    });
}

static void render_bla(wacfrac::BLAOptions& opts) {
    auto max_iterations {get_max_iterations(opts)};
    RenderConfig cfg{max_iterations, opts.escape_radius, &opts.palette, opts.continuous_coloring};
    render_image(opts, [&, cfg]<typename T>(const auto& view, auto& pixels, NumericTypeTag<T>){
        using CT = wacfrac::ComplexValueTypeT<T>;
        auto c_ref = view.center;
        auto ref = wacfrac::compute_reference_mt<T>(c_ref, max_iterations, std::numeric_limits<double>::infinity());
        wacfrac::logging::info(
            "Reference at view center with orbit length {}", ref.size());
        auto last_level = static_cast<std::size_t>(std::log2(ref.size()));
        auto first_level = opts.first_level != 0
            ? opts.first_level
            : std::max(0uz, last_level > 9 ? last_level - 9 : 0uz);
        auto probes = view.template generate_probes<T>(opts.probe_grid.first, opts.probe_grid.second);
        auto max_dc = wacfrac::to_complex<T>(view.compute_max_dc(c_ref));
        wacfrac::BivariateLinearApproximator<T> bla{
            opts.epsilon != 0.0
                ? wacfrac::BivariateLinearApproximator<T>{static_cast<CT>(opts.epsilon), max_dc, ref, first_level, opts.escape_radius}
                : wacfrac::BivariateLinearApproximator<T>{opts.lower_exp, opts.upper_exp, opts.tolerance, probes, max_dc, ref, first_level, opts.escape_radius}
        };
        bla_render_pass<T>(pixels, opts.resolution, view, cfg, bla, c_ref);
    });
}

static void render_video(wacfrac::VideoOptions& opts) {
    auto final_view = make_viewport(opts.focus, opts.final_scale, opts.resolution);
    auto max_iterations = final_view.required_iterations();
    auto precision = final_view.required_precision();

    wacfrac::MultiFloat::default_precision(precision);
    wacfrac::MultiComplex::default_precision(precision);

    auto pick_type = [](auto p) -> std::string {
        if (p > 1000) return "dexp"; // NOTE: Imprecise
        if (p > 230) return "long-double";
        return "double";
    };

    auto c_ref = opts.focus;
    auto refs = wacfrac::compute_references_all(c_ref, max_iterations, std::numeric_limits<double>::infinity());
    auto ref_size = refs.double_ref.size();
    auto last_level = static_cast<std::size_t>(std::log2(ref_size));
    auto first_level = std::max(0uz, last_level > 9 ? last_level - 9 : 0uz);

    wacfrac::logging::info(
        "Video pre-compute: final_zoom={} max_iterations={} precision={} ref_size={}",
        opts.final_scale, max_iterations, precision, ref_size);

    constexpr double DIRECT_THRESHOLD = 1e13;
    constexpr double PERTURB_THRESHOLD = 1e25;

    // I am sorry for this function call.
    wacfrac::write_zoom_frames(
        opts.directory, opts.segment_size, opts.resolution,
        opts.initial_scale, opts.final_scale,
        opts.zoom_per_second, opts.frames_per_second,
        [&](std::span<wacfrac::Pixel> pixels, wacfrac::MultiFloat scale) {
            auto view = make_viewport(opts.focus, scale, opts.resolution);
            auto frame_precision = view.required_precision();
            auto frame_max_iter = view.required_iterations(
                std::get<0>(opts.iteration_parameters),
                std::get<1>(opts.iteration_parameters),
                std::get<2>(opts.iteration_parameters)
            );

            wacfrac::MultiFloat::default_precision(frame_precision);
            wacfrac::MultiComplex::default_precision(frame_precision);
            view.precision(frame_precision);

            RenderConfig cfg{frame_max_iter, opts.escape_radius, &opts.palette, opts.continuous_coloring};

            auto scale_d = static_cast<double>(scale);
            auto ft = pick_type(frame_precision);

            auto render_perturbed_or_bla = [&](auto tag, const auto& ref) {
                using T = typename decltype(tag)::type;
                if (scale_d <= PERTURB_THRESHOLD) {
                    wacfrac::logging::info(
                        "Frame zoom={} algorithm=perturbed", scale_d);
                    perturbed_render_pass<T>(pixels, opts.resolution, view, cfg, ref, c_ref);
                } else {
                    wacfrac::logging::info(
                        "Frame zoom={} algorithm=BLA", scale_d);
                    auto probes = view.template generate_probes<T>(3, 3);
                    auto max_dc = wacfrac::to_complex<T>(view.compute_max_dc(c_ref));
                    wacfrac::BivariateLinearApproximator<T> bla{
                        static_cast<double>(-(1 << 12)), static_cast<double>(-(1 << 6)),
                        1e-10, probes, max_dc, ref, first_level,
                        opts.escape_radius
                    };
                    bla_render_pass<T>(pixels, opts.resolution, view, cfg, bla, c_ref);
                }
            };

            if (scale_d <= DIRECT_THRESHOLD) {
                wacfrac::logging::info(
                    "Frame zoom={} algorithm=direct", scale_d);
                with_numeric_type(ft, [&](auto tag) {
                    using T = typename decltype(tag)::type;
                    direct_render_pass<T>(pixels, opts.resolution, view, cfg);
                });
            } else {
                with_numeric_type(ft, [&](auto tag) {
                    using T = typename decltype(tag)::type;
                    if constexpr (std::is_same_v<T, std::complex<double>>)
                        render_perturbed_or_bla(tag, refs.double_ref);
                    else if constexpr (std::is_same_v<T, std::complex<long double>>)
                        render_perturbed_or_bla(tag, refs.long_double_ref);
                    else
                        render_perturbed_or_bla(tag, refs.dexp_ref);
                });
            }
        }
    );
}

static void dispatch_render(argumentum::CommandOptions* cmd) {
    using namespace wacfrac;

    if (auto* p = dynamic_cast<DirectOptions*>(cmd))    return render_direct(*p);
    if (auto* p = dynamic_cast<PerturbedOptions*>(cmd)) return render_perturbed(*p);
    if (auto* p = dynamic_cast<BLAOptions*>(cmd))       return render_bla(*p);
    if (auto* p = dynamic_cast<VideoOptions*>(cmd))     return render_video(*p);
}

int main(int argc, char* argv[])
{
    auto parser = argumentum::argument_parser{};
    auto params = parser.params();
    parser.config().program(argv[0]).description("Mandelbrot Set Plotter");

    params.add_command<wacfrac::DirectOptions>("direct").help("Direct escape-time rendering");
    params.add_command<wacfrac::PerturbedOptions>("perturbed").help("Perturbation-theory rendering");
    params.add_command<wacfrac::BLAOptions>("bla").help("Bivariate linear approximation rendering");
    params.add_command<wacfrac::VideoOptions>("video").help("Sequential rendering (various methods)");

    auto parse_result {parser.parse_args(argc, argv)};
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
    wacfrac::logging::info( "Mandelbrot Set Plotter");
    dispatch_render(cmd.get());
}
