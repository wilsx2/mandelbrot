#include "wacfrac/cli_options.hpp"
#include "wacfrac/constants.hpp"
#include "wacfrac/types.hpp"
#include "wacfrac/wacfrac.hpp"
#include <atomic>
#include <chrono>
#include <cmath>
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

template <typename T>
void render_direct(
    const wacfrac::SharedOptions& opts,
    const wacfrac::Viewport& view,
    const wacfrac::Resolution& resolution,
    const std::vector<wacfrac::Pixel>& palette,
    std::size_t max_iterations,
    std::span<wacfrac::Pixel> pixels)
{
    wacfrac::absolute_render<T>(pixels, resolution, view,
        [max_iterations, escape_radius = opts.escape_radius](T c) {
            return wacfrac::escape<T>(c, max_iterations, escape_radius);
        },
        make_color_fn(palette, opts.continuous_coloring, max_iterations)
    );
}

template <typename T>
void render_perturbed(
    const wacfrac::SharedOptions& opts,
    const wacfrac::Viewport& view,
    const wacfrac::Resolution& resolution,
    const std::vector<wacfrac::Pixel>& palette,
    std::size_t max_iterations,
    wacfrac::MultiComplex c_ref,
    std::span<wacfrac::Pixel> pixels)
{
    auto ref {wacfrac::compute_reference_mt<T>(c_ref, max_iterations, opts.escape_radius)};
    wacfrac::logging::print(wacfrac::logging::Severity::Info,
        "Reference at view center with orbit length {}", ref.size());

    wacfrac::perturbed_render<T>(pixels, resolution, view,
        [&ref, max_iterations, escape_radius = opts.escape_radius](T dc) {
            return wacfrac::escape_perturbed<T>(ref, dc, max_iterations, escape_radius);
        },
        make_color_fn(palette, opts.continuous_coloring, max_iterations),
        c_ref);
}

template <typename T>
void render_sa(
    const wacfrac::SAOptions& opts,
    const wacfrac::Viewport& view,
    const wacfrac::Resolution& resolution,
    const std::vector<wacfrac::Pixel>& palette,
    std::size_t max_iterations,
    wacfrac::MultiComplex c_ref,
    std::span<wacfrac::Pixel> pixels)
{
    auto ref {wacfrac::compute_reference_mt<T>(c_ref, max_iterations, opts.shared->escape_radius)};
    wacfrac::logging::print(wacfrac::logging::Severity::Info,
        "Reference at view center with orbit length {}", ref.size());

    auto probes = view.generate_probes<T>(opts.probes[0], opts.probes[1]);

    wacfrac::SeriesApproximator<T> sa{ref, opts.coeffs, probes, opts.validity_threshold, opts.shared->escape_radius};

    wacfrac::perturbed_render<T>(pixels, resolution, view,
        [&sa](T dc) {
            auto [z, n] = sa.approximate_escape(dc);
            return std::make_tuple(z, n, std::size_t{0});
        },
        make_color_fn(palette, opts.shared->continuous_coloring, max_iterations),
        c_ref);
}

template <typename T>
void render_bla(
    const wacfrac::BLAOptions& opts,
    const wacfrac::Viewport& view,
    const wacfrac::Resolution& resolution,
    const std::vector<wacfrac::Pixel>& palette,
    std::size_t max_iterations,
    wacfrac::MultiComplex c_ref,
    std::span<wacfrac::Pixel> pixels)
{
    using CT = wacfrac::ComplexValueTypeT<T>;

    auto ref {wacfrac::compute_reference_mt<T>(c_ref, max_iterations, opts.shared->escape_radius)};
    wacfrac::logging::print(wacfrac::logging::Severity::Info,
        "Reference at view center with orbit length {}", ref.size());

    auto last_level {static_cast<std::size_t>(std::log2(ref.size()))};
    auto first_level {opts.first_level != 0
        ? opts.first_level
        : std::max(0uz, last_level > 9 ? last_level - 9 : 0uz)};
    auto probes {view.generate_probes<T>(opts.probes[0], opts.probes[1])};
    auto max_dc {wacfrac::to_complex<T>(view.compute_max_dc(c_ref))};

    wacfrac::BivariateLinearApproximator<T> bla{
        opts.epsilon != 0.0
            ? wacfrac::BivariateLinearApproximator<T>{static_cast<CT>(opts.epsilon), max_dc, ref, first_level, opts.shared->escape_radius}
            : wacfrac::BivariateLinearApproximator<T>{static_cast<double>(opts.lower_exp), static_cast<double>(opts.upper_exp), opts.tolerance, probes, max_dc, ref, first_level, opts.shared->escape_radius}
    };

    auto total_skipped = std::atomic<std::uint64_t>{0};

    wacfrac::perturbed_render<T>(pixels, resolution, view,
        [&bla, &total_skipped](T dc) {
            auto result {bla.escape_approximate(dc)};
            total_skipped += std::get<2>(result);
            return result;
        },
        make_color_fn(palette, opts.shared->continuous_coloring, max_iterations),
        c_ref);

    auto avg_skipped {static_cast<double>(total_skipped) / resolution.area()};
    wacfrac::logging::print(wacfrac::logging::Severity::Info, "BLA render complete (avg skipped: {})", avg_skipped);
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
    params.add_command<wacfrac::SAOptions>("sa").help("Series approximation rendering");
    params.add_command<wacfrac::BLAOptions>("bla").help("Bivariate linear approximation rendering");

    auto parse_result {parser.parse_args(argc, argv)};
    if (!parse_result)
        return EXIT_FAILURE;

    std::shared_ptr<argumentum::CommandOptions> cmd;
    for (auto& pcmd : parse_result.commands)
        if (pcmd) { cmd = pcmd; break; }
    if (!cmd) {
        wacfrac::logging::print(wacfrac::logging::Severity::Error,
            "No render mode specified. Use one of: direct, perturbed, sa, bla");
        return EXIT_FAILURE;
    }

    const wacfrac::SharedOptions* shared {nullptr};
    if (auto* p = dynamic_cast<wacfrac::DirectOptions*>(cmd.get())) shared = p->shared.get();
    else if (auto* p = dynamic_cast<wacfrac::PerturbedOptions*>(cmd.get())) shared = p->shared.get();
    else if (auto* p = dynamic_cast<wacfrac::SAOptions*>(cmd.get())) shared = p->shared.get();
    else if (auto* p = dynamic_cast<wacfrac::BLAOptions*>(cmd.get())) shared = p->shared.get();

    if (!shared) {
        wacfrac::logging::print(wacfrac::logging::Severity::Error, "Unknown render mode");
        return EXIT_FAILURE;
    }

    const auto& opts = *shared;

    wacfrac::logging::print(wacfrac::logging::Severity::Info,
        "Configuration: output={} dimensions={}x{} focus=({}, {}) zoom={} max_iterations={} precision={} numeric_type={}",
        opts.filepath, opts.dimensions[0], opts.dimensions[1],
        opts.focus[0], opts.focus[1], opts.scale,
        opts.max_iterations, opts.precision, opts.numeric_type);

    std::vector<wacfrac::Pixel> palette {};
    std::ranges::copy(
        opts.palette | std::views::transform([](std::string_view string) {
            return wacfrac::parse_color(string);
        }),
        std::back_inserter(palette)
    );
    if (palette.empty()) {
        wacfrac::logging::print(wacfrac::logging::Severity::Info,
            "Falling back to default palette");
        palette = wacfrac::palette::ULTRA;
    }

    wacfrac::MultiFloat zoom_scale {opts.scale, 1000};

    wacfrac::Resolution resolution {opts.dimensions[0], opts.dimensions[1]};

    wacfrac::Viewport view;
    view.center = wacfrac::MultiComplex{opts.focus[0], opts.focus[1], 10000};// NOTE: Arbitrily chosen digit count
    auto aspect_ratio {resolution.width / static_cast<double>(resolution.height)};
    if (aspect_ratio >= 1.0) {
        view.dimensions = {aspect_ratio, 1.0};
    } else {
        view.dimensions = {1.0, 1.0/aspect_ratio};
    }
    view = view.zoomed(zoom_scale);

    auto max_iterations {opts.max_iterations};
    auto precision {opts.precision};
    if (max_iterations == 0)
        max_iterations = view.required_iterations();
    if (precision == 0)
        precision = view.required_precision();

    wacfrac::MultiFloat::default_precision(precision);
    wacfrac::MultiComplex::default_precision(precision);
    view.precision(precision);
    zoom_scale.precision(precision);

    wacfrac::logging::print(wacfrac::logging::Severity::Info,
        "Viewport: center=({}) dimensions=({})", view.center, view.dimensions);
    wacfrac::logging::print(wacfrac::logging::Severity::Info,
        "Resolution: {}x{} ({} pixels)", resolution.width, resolution.height, resolution.area());
    wacfrac::logging::print(wacfrac::logging::Severity::Info,
        "Using {} max iterations with {} decimal digits precision", max_iterations, precision);

    auto c_ref {view.center};
    std::vector<wacfrac::Pixel> pixels {resolution.area()};

    auto t_render {std::chrono::steady_clock::now()};

    if (dynamic_cast<wacfrac::DirectOptions*>(cmd.get())) {
        wacfrac::logging::print(wacfrac::logging::Severity::Info,
            "Rendering {} pixels (direct)...", resolution.area());
        with_numeric_type(opts.numeric_type, [&]<typename T>(NumericTypeTag<T>) {
            render_direct<T>(opts, view, resolution, palette, max_iterations, pixels);
        });
    } else if (dynamic_cast<wacfrac::PerturbedOptions*>(cmd.get())) {
        wacfrac::logging::print(wacfrac::logging::Severity::Info,
            "Rendering {} pixels (perturbed)...", resolution.area());
        with_numeric_type(opts.numeric_type, [&]<typename T>(NumericTypeTag<T>) {
            render_perturbed<T>(opts, view, resolution, palette, max_iterations, c_ref, pixels);
        });
    } else if (auto* sa_opts = dynamic_cast<wacfrac::SAOptions*>(cmd.get())) {
        wacfrac::logging::print(wacfrac::logging::Severity::Info,
            "Rendering {} pixels (SA)...", resolution.area());
        with_numeric_type(opts.numeric_type, [&]<typename T>(NumericTypeTag<T>) {
            render_sa<T>(*sa_opts, view, resolution, palette, max_iterations, c_ref, pixels);
        });
    } else if (auto* bla_opts = dynamic_cast<wacfrac::BLAOptions*>(cmd.get())) {
        wacfrac::logging::print(wacfrac::logging::Severity::Info,
            "Rendering {} pixels (BLA)...", resolution.area());
        with_numeric_type(opts.numeric_type, [&]<typename T>(NumericTypeTag<T>) {
            render_bla<T>(*bla_opts, view, resolution, palette, max_iterations, c_ref, pixels);
        });
    }

    auto render_ms {std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t_render)};
    wacfrac::logging::print(wacfrac::logging::Severity::Info, "Render took {}ms", render_ms.count());

    wacfrac::write_ppm(opts.filepath, resolution, pixels);
}
