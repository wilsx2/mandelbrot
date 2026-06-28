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
    wacfrac::logging::print(wacfrac::logging::Severity::Error,
        "Unknown numeric type: {}", type);
    std::exit(EXIT_FAILURE);
}

auto make_view(wacfrac::ImageOptions& opt) {
    wacfrac::Viewport view;

    view.center = opt.focus;
    auto aspect_ratio {opt.resolution.width / static_cast<double>(opt.resolution.height)};
    if (aspect_ratio >= 1.0) {
        view.dimensions = {aspect_ratio, 1.0};
    } else {
        view.dimensions = {1.0, 1.0/aspect_ratio};
    }
    view = view.zoomed(opt.scale);

    if (opt.precision == 0)
        opt.precision = view.required_precision();
    if (opt.max_iterations == 0)
        opt.max_iterations = view.required_iterations();

    wacfrac::MultiFloat::default_precision(opt.precision);
    wacfrac::MultiComplex::default_precision(opt.precision);
    view.precision(opt.precision);

    wacfrac::logging::print(wacfrac::logging::Severity::Info,
        "Configuration: output={} resolution={}x{} focus=({}, {}) zoom={} max_iterations={} precision={} numeric_type={}",
        opt.filepath, opt.resolution.width, opt.resolution.height,
        opt.focus.real(), opt.focus.imag(), opt.scale,
        opt.max_iterations, opt.precision, opt.numeric_type);

    return view;
}

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

    wacfrac::logging::print(wacfrac::logging::Severity::Info, "Render took {}ms", render_ms.count());
    wacfrac::write_ppm(opts.filepath, opts.resolution, pixels);
}

static void render_direct(wacfrac::DirectOptions& opts) {
    render_image(opts, [&]<typename T>(const auto& view, auto& pixels, NumericTypeTag<T>){
        wacfrac::absolute_render<T>(pixels, opts.resolution, view,
        [max_iterations = opts.max_iterations,
            escape_radius = opts.escape_radius](T c) {
                return wacfrac::escape<T>(c, max_iterations, escape_radius);
            },
            make_color_fn(opts.palette, opts.continuous_coloring, opts.max_iterations)
        );
    }); 
}
static void render_perturbed(wacfrac::PerturbedOptions& opts) {
    render_image(opts, [&opts]<typename T>(const auto& view, auto& pixels, NumericTypeTag<T>){
        auto c_ref = view.center;
        auto ref {wacfrac::compute_reference_mt<T>(c_ref, opts.max_iterations, opts.escape_radius)};
        wacfrac::logging::print(wacfrac::logging::Severity::Info,
            "Reference at view center with orbit length {}", ref.size());
        wacfrac::perturbed_render<T>(pixels, opts.resolution, view,
            [&ref, max_iterations = opts.max_iterations, escape_radius = opts.escape_radius](T dc) {
                return wacfrac::escape_perturbed<T>(ref, dc, max_iterations, escape_radius);
            },
            make_color_fn(opts.palette, opts.continuous_coloring, opts.max_iterations),
            c_ref);
    });
}

static void render_bla(wacfrac::BLAOptions& opts) {
    render_image(opts, [&opts]<typename T>(const auto& view, auto& pixels, NumericTypeTag<T>){
        using CT = wacfrac::ComplexValueTypeT<T>;
        auto c_ref = view.center;
        auto ref {wacfrac::compute_reference_mt<T>(c_ref, opts.max_iterations, opts.escape_radius)};
        wacfrac::logging::print(wacfrac::logging::Severity::Info,
            "Reference at view center with orbit length {}", ref.size());
        auto last_level {static_cast<std::size_t>(std::log2(ref.size()))};
        auto first_level {opts.first_level != 0
            ? opts.first_level
            : std::max(0uz, last_level > 9 ? last_level - 9 : 0uz)};
        auto probes {view.template generate_probes<T>(opts.probe_grid.first, opts.probe_grid.second)};
        auto max_dc {wacfrac::to_complex<T>(view.compute_max_dc(c_ref))};
        wacfrac::BivariateLinearApproximator<T> bla{
            opts.epsilon != 0.0
                ? wacfrac::BivariateLinearApproximator<T>{static_cast<CT>(opts.epsilon), max_dc, ref, first_level, opts.escape_radius}
                : wacfrac::BivariateLinearApproximator<T>{static_cast<double>(opts.lower_exp), static_cast<double>(opts.upper_exp), opts.tolerance, probes, max_dc, ref, first_level, opts.escape_radius}
        };
        auto total_skipped = std::atomic<std::uint64_t>{0};
        wacfrac::perturbed_render<T>(pixels, opts.resolution, view,
            [&bla, &total_skipped](T dc) {
                auto result {bla.escape_approximate(dc)};
                total_skipped += std::get<2>(result);
                return result;
            },
            make_color_fn(opts.palette, opts.continuous_coloring, opts.max_iterations),
            c_ref);
        auto avg_skipped {static_cast<double>(total_skipped) / opts.resolution.area()};
        wacfrac::logging::print(wacfrac::logging::Severity::Info, "BLA render complete (avg skipped: {})", avg_skipped);
    });
}

static void dispatch_render(argumentum::CommandOptions* cmd) {
    using namespace wacfrac;

    if (auto* p = dynamic_cast<DirectOptions*>(cmd))    return render_direct(*p);
    if (auto* p = dynamic_cast<PerturbedOptions*>(cmd)) return render_perturbed(*p);
    if (auto* p = dynamic_cast<BLAOptions*>(cmd))       return render_bla(*p);
}

int main(int argc, char* argv[])
{
    wacfrac::logging::init();
    wacfrac::logging::print(wacfrac::logging::Severity::Info, "Mandelbrot Set Plotter");

    auto parser = argumentum::argument_parser{};
    auto params = parser.params();
    parser.config().program(argv[0]).description("Mandelbrot Set Plotter");

    params.add_command<wacfrac::DirectOptions>("direct").help("Direct escape-time rendering");
    params.add_command<wacfrac::PerturbedOptions>("perturbed").help("Perturbation-theory rendering");
    params.add_command<wacfrac::BLAOptions>("bla").help("Bivariate linear approximation rendering");

    auto parse_result {parser.parse_args(argc, argv)};
    if (!parse_result)
        return EXIT_FAILURE;

    std::shared_ptr<argumentum::CommandOptions> cmd;
    for (auto& pcmd : parse_result.commands)
        if (pcmd) { cmd = pcmd; break; }
    if (!cmd) {
        wacfrac::logging::print(wacfrac::logging::Severity::Error,
            "No render mode specified. Use one of: direct, perturbed, bla");
        return EXIT_FAILURE;
    }

    dispatch_render(cmd.get());
}
