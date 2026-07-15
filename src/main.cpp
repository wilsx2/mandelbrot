#include "wacfrac/cli_options.hpp"
#include "wacfrac/context.hpp"
#include "wacfrac/io.hpp"
#include "wacfrac/log.hpp"
#include <cstdlib>
#include <string_view>

namespace {
/*
void render_video(wacfrac::VideoOptions& opts) {
    if (!std::filesystem::create_directory(opts.directory)) {
        wacfrac::logging::error("Directory '{}' failed to create", opts.directory);
    }
    std::filesystem::current_path(opts.directory);

    auto max_iterations = wacfrac::required_iterations(opts.final_scale);
    auto precision = wacfrac::required_precision(opts.final_scale);
    wacfrac::MultiFloat::default_precision(static_cast<unsigned>(precision));
    wacfrac::MultiComplex::default_precision(static_cast<unsigned>(precision));
    wacfrac::Viewport final_view {opts.shared->focus, opts.final_scale, opts.shared->resolution};

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
*/

} // namespace

int main(int argc, char* argv[])
{
    wacfrac::logging::init(0);

    auto parser = argumentum::argument_parser{};
    auto params = parser.params();
    parser.config().program(argv[0]).description("Mandelbrot Set Plotter");

    wacfrac::RendererOptions<wacfrac::Host> renderer_opts;
    renderer_opts.add_parameters(params);

    params.add_command<wacfrac::ImageOptions>("image");
    // params.add_command<wacfrac::VideoOptions>("video");

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

    wacfrac::logging::log_level() = renderer_opts.log_level;
    wacfrac::Renderer<wacfrac::Host> renderer {std::move(renderer_opts)};

    if (auto* img_opts = dynamic_cast<wacfrac::ImageOptions*>(cmd.get())) {
        if (wacfrac::write_ppm(img_opts->filepath, renderer.conf.resolution, renderer.render(*img_opts))) {
            wacfrac::logging::info("Image written to {}", img_opts->filepath);
        } else {
            wacfrac::logging::error("Image write failed");
        }
    }
    /*
    if (auto* p = dynamic_cast<wacfrac::VideoOptions*>(cmd.get())) {
        wacfrac::logging::init(p->shared->log_level);
        render_video(*p);
    }
    */
}
