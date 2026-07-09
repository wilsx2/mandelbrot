#include "wacfrac/bla.hpp"
#include "wacfrac/cli_options.hpp"
#include "wacfrac/color.hpp"
#include "wacfrac/io.hpp"
#include "wacfrac/log.hpp"
#include "wacfrac/orbit.hpp"
#include "wacfrac/rendering.hpp"
#include "wacfrac/viewport.hpp"
#include "wacfrac/wacfrac.hpp"
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <limits>
#include <string_view>

namespace {

template <typename T>
struct NumericTypeTag { using type = T; };

template <typename F>
decltype(auto) with_numeric_type(const std::string& type, F&& f) {
    if (type == "float")
        return f(NumericTypeTag<std::complex<float>>{});
    if (type == "double")
        return f(NumericTypeTag<std::complex<double>>{});
    if (type == "dexp")
        return f(NumericTypeTag<wacfrac::DoubleExpComplex>{});
    wacfrac::logging::error("Unknown numeric type '{}'", type);
    std::exit(EXIT_FAILURE);
}

void render_image(const wacfrac::ImageOptions& opts) {
    auto max_n {opts.effective_max_iterations()};
    auto num_type {opts.effective_numeric_type()};
    auto render_type {opts.effective_render_type()};
    auto p {opts.effective_precision()};
    wacfrac::logging::info(
        "{}: resolution={}x{} focus=({}, {}) zoom={} max_iterations={} precision={} numeric_type={} render_type={}",
        opts.filepath, opts.shared->resolution.width, opts.shared->resolution.height,
        opts.shared->focus.real(), opts.shared->focus.imag(), opts.scale,
        max_n, p, num_type, [&](){
            switch (render_type) {
                case wacfrac::RenderType::Direct: return "direct";
                case wacfrac::RenderType::Perturbed: return "perturbed";
                case wacfrac::RenderType::BLA: return "bla";
            }
            return "???";
        }());

    wacfrac::MultiFloat::default_precision(static_cast<unsigned>(p));
    wacfrac::MultiComplex::default_precision(static_cast<unsigned>(p));

    std::vector<wacfrac::Pixel> pixels(opts.shared->resolution.area());

    auto start = std::chrono::steady_clock::now();
    with_numeric_type(num_type, [&]<typename T>(NumericTypeTag<T>){
        std::vector<std::pair<std::complex<float>, unsigned>> escaped_orbits;
        escaped_orbits.reserve(pixels.size());

        if (render_type == wacfrac::RenderType::Direct) {
            wacfrac::Viewport v {opts.shared->focus, opts.scale, opts.shared->resolution};
            v.precision(p);
            auto cs {wacfrac::sample_c_values<T>(v, opts.shared->resolution)};
            for (auto c : cs)
                escaped_orbits.push_back(wacfrac::escape(c, max_n, opts.shared->escape_radius));
        } else {
            wacfrac::Viewport view {opts.shared->focus, opts.scale, opts.shared->resolution};
            view.precision(p);
            auto c_ref {opts.shared->focus};
            const auto& ref {
                opts.ref_set.has_value()
                ? opts.ref_set->select<T>()
                : wacfrac::compute_reference_mt<T>(
                    c_ref, max_n, 
                    std::numeric_limits<double>::infinity())
            };
            auto dcs {wacfrac::sample_c_values<T>(
                view, opts.shared->resolution,
                wacfrac::to_complex<T>(c_ref)
            )};

            if (render_type == wacfrac::RenderType::Perturbed) {
                for (auto dc : dcs)
                    escaped_orbits.push_back(wacfrac::escape_perturbed(dc, std::span<const T>(ref), max_n, opts.shared->escape_radius));
            } else if (render_type == wacfrac::RenderType::BLA) {
                using CT = wacfrac::ComplexValueTypeT<T>;
                auto max_dc {wacfrac::to_complex<T>(view.compute_max_dc(c_ref))};
                wacfrac::BivariateLinearApproximator<T> bla {
                    opts.epsilon != 0.0
                    ? wacfrac::BivariateLinearApproximator<T>{
                        static_cast<CT>(opts.epsilon), max_dc,
                        ref, opts.shared->first_level, opts.shared->escape_radius}
                    : wacfrac::BivariateLinearApproximator<T>{
                        opts.shared->lower_exp, opts.shared->upper_exp, opts.shared->tolerance,
                        view.generate_probes<T>(opts.shared->probe_grid.first, opts.shared->probe_grid.second),
                        max_dc, ref, opts.shared->first_level, opts.shared->escape_radius
                    }
                };
                for (auto dc : dcs) {
                    auto [z, n, _] = bla.escape_approximate(dc);
                    escaped_orbits.emplace_back(z, n);
                }
            }
        }

        for (auto&& [pixel, orbit] : std::views::zip(pixels, escaped_orbits)) {
            pixel = opts.shared->discrete_coloring
                ? wacfrac::colorize_discrete(std::get<1>(orbit), max_n, opts.shared->palette)
                : wacfrac::colorize_continuous(static_cast<std::complex<float>>(std::get<0>(orbit)), std::get<1>(orbit), max_n, opts.shared->palette);
        } 
    });
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start
    );

    wacfrac::logging::info("Image render took {}ms", elapsed.count());
    if (wacfrac::write_ppm(opts.filepath, opts.shared->resolution, pixels)) {
        wacfrac::logging::info("Image written to {}", opts.filepath);
    } else {
        wacfrac::logging::error("Image write failed");
    }
}

void render_video(wacfrac::VideoOptions& opts) {
    if (!std::filesystem::create_directory(opts.directory)) {
        wacfrac::logging::error("Directory '{}' failed to create", opts.directory);
    }
    std::filesystem::current_path(opts.directory);

    wacfrac::Viewport final_view {opts.shared->focus, opts.final_scale, opts.shared->resolution};
    auto max_iterations = final_view.required_iterations();
    auto precision = final_view.required_precision();

    wacfrac::MultiFloat::default_precision(static_cast<unsigned>(precision));
    wacfrac::MultiComplex::default_precision(static_cast<unsigned>(precision));

    auto refs = wacfrac::compute_reference_set(
        opts.shared->focus,
        max_iterations,
        std::numeric_limits<double>::infinity());

    wacfrac::logging::info(
        "Video pre-compute: final_zoom={} max_iterations={} precision={} ref_size={}",
        opts.final_scale, max_iterations, precision, refs.double_ref.size());
    auto total_frames {wacfrac::total_frames(
        opts.initial_scale, opts.final_scale, 
        opts.zoom_per_second, static_cast<float>(opts.frames_per_second))};
    auto total_segments {total_frames / opts.segment_size};
    auto scales {wacfrac::frame_zooms(
        opts.initial_scale, opts.final_scale, 
        opts.zoom_per_second, static_cast<float>(opts.frames_per_second))};
    for (auto&& [frame, scale] : std::views::enumerate(std::move(scales))) {
        auto segment = frame / opts.segment_size;
        auto frame_filename {"frame_" + wacfrac::file_suffix(frame % opts.segment_size, opts.segment_size) + ".ppm"};
        auto segment_filename {"segment_" + wacfrac::file_suffix(segment, total_segments) + ".mp4"};

        if (!std::filesystem::exists(segment_filename)) {
            if (!std::filesystem::exists(frame_filename)) {
                wacfrac::logging::info("Frame #{}/{} being rendered", frame, total_frames);
                wacfrac::ImageOptions frame_opts("");
                frame_opts.shared = opts.shared;
                frame_opts.ref_set = refs;
                frame_opts.filepath = "frame_" + wacfrac::file_suffix(frame % opts.segment_size, opts.segment_size) + ".ppm";
                frame_opts.scale = scale;
                render_image(frame_opts);
            } else {
                wacfrac::logging::debug( "Frame #{} has already been rendered; skipping", frame);
            }
            if (frame != 0 && (frame % opts.segment_size == opts.segment_size - 1
            || static_cast<std::size_t>(frame) == total_frames - 1)) {
                auto status {wacfrac::concatenate_images(
                    segment_filename,
                    "frame_%" + wacfrac::file_suffix_format(opts.segment_size) + ".ppm",
                    static_cast<float>(opts.frames_per_second)
                )};
                if (status) {
                    for (auto& entry : std::filesystem::directory_iterator("."))
                        if (auto name = entry.path().filename().string();
                            name.starts_with("frame_") && name.ends_with(".ppm"))
                            std::filesystem::remove(entry.path());
                    wacfrac::logging::info("Segment #{} composed", segment);
                } else {
                    wacfrac::logging::error("Segment #{} failed to compose", segment);
                }
            }
        } else {
            wacfrac::logging::debug( "Frame #{} has already been rendered; skipping", frame);
        }
    }
    auto status {wacfrac::concatenate_videos("final.mp4", "segment_*.mp4")};
    if (status) {
        for (auto& entry : std::filesystem::directory_iterator("."))
            if (auto name = entry.path().filename().string();
                name.starts_with("segment_") && name.ends_with(".mp4"))
                std::filesystem::remove(entry.path());
        wacfrac::logging::info("Video render complete");
    } else {
        wacfrac::logging::error("Final video failed to compose");
    }
}

} // namespace

int main(int argc, char* argv[])
{
    auto parser = argumentum::argument_parser{};
    auto params = parser.params();
    parser.config().program(argv[0]).description("Mandelbrot Set Plotter");

    params.add_command<wacfrac::ImageOptions>("image").help("Single render");
    params.add_command<wacfrac::VideoOptions>("video").help("Multi-render video");

    auto parse_result = parser.parse_args(argc, argv);
    if (!parse_result)
        return EXIT_FAILURE;

    std::shared_ptr<argumentum::CommandOptions> cmd;
    for (auto& pcmd : parse_result.commands)
        if (pcmd) { cmd = pcmd; break; }
    if (!cmd) {
        wacfrac::logging::error(
            "No render mode specified. Use either image or video");
        return EXIT_FAILURE;
    }

    if (auto* p = dynamic_cast<wacfrac::ImageOptions*>(cmd.get())) {
        wacfrac::logging::init(p->shared->log_level);
        render_image(*p);
    }
    if (auto* p = dynamic_cast<wacfrac::VideoOptions*>(cmd.get())) {
        wacfrac::logging::init(p->shared->log_level);
        render_video(*p);
    }
}
