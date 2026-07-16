#include "wacfrac/cli_options.hpp"
#include "wacfrac/context.hpp"
#include "wacfrac/viewport.hpp"
#include "wacfrac/reference.hpp"
#include "wacfrac/io.hpp"
#include "wacfrac/log.hpp"
#include <cstdlib>
#include <string_view>

namespace {

bool has_gpu() {
    int count = 0;
    auto err = cudaGetDeviceCount(&count);
    return err == cudaSuccess && count > 0;
}

template<typename Context>
void save_render(wacfrac::Renderer<Context>& renderer, const wacfrac::ImageOptions& img_opts) {
    if (wacfrac::write_ppm(img_opts.filepath, renderer.conf.resolution, renderer.render(img_opts))) {
        wacfrac::logging::info("Image written to {}", img_opts.filepath);
    } else {
        wacfrac::logging::error("Image write failed");
    }
}

template<typename Context>
void render_video(wacfrac::Renderer<Context>& renderer, wacfrac::VideoOptions& vid_opts) {
    if (!std::filesystem::create_directory(vid_opts.directory)) {
        wacfrac::logging::error("Directory '{}' failed to create", vid_opts.directory);
    }
    std::filesystem::current_path(vid_opts.directory);

    auto max_iterations = wacfrac::required_iterations(
        vid_opts.final_scale,
        renderer.conf.iteration_parameters.modifier,
        renderer.conf.iteration_parameters.factor,
        renderer.conf.iteration_parameters.exponent);
    auto precision = wacfrac::required_precision(vid_opts.final_scale);
    wacfrac::MultiFloat::default_precision(static_cast<unsigned>(precision));
    wacfrac::MultiComplex::default_precision(static_cast<unsigned>(precision));
    wacfrac::Viewport final_view {renderer.conf.focus, vid_opts.final_scale, renderer.conf.resolution};

    auto refs = wacfrac::compute_reference_set(
        renderer.conf.focus,
        max_iterations,
        std::numeric_limits<double>::infinity());

    wacfrac::logging::info(
        "Video pre-compute: final_zoom={} max_iterations={} precision={} ref_size={}",
        vid_opts.final_scale, max_iterations, precision, refs.double_ref.size());
    auto total_frames {wacfrac::total_frames(
        vid_opts.initial_scale, vid_opts.final_scale, 
        vid_opts.zoom_per_second, static_cast<float>(vid_opts.frames_per_second))};
    auto total_segments {total_frames / vid_opts.segment_size};
    auto scales {wacfrac::frame_zooms(
        vid_opts.initial_scale, vid_opts.final_scale, 
        vid_opts.zoom_per_second, static_cast<float>(vid_opts.frames_per_second))};
    for (auto&& [frame, scale] : std::views::enumerate(std::move(scales))) {
        auto segment = frame / vid_opts.segment_size;
        auto frame_filename {"frame_" + wacfrac::file_suffix(frame % vid_opts.segment_size, vid_opts.segment_size) + ".ppm"};
        auto segment_filename {"segment_" + wacfrac::file_suffix(segment, total_segments) + ".mp4"};

        if (!std::filesystem::exists(segment_filename)) {
            if (!std::filesystem::exists(frame_filename)) {
                wacfrac::logging::info("Frame #{}/{} being rendered", frame, total_frames);
                wacfrac::ImageOptions frame_opts {""};
                frame_opts.ref_set = refs;
                frame_opts.filepath = "frame_" + wacfrac::file_suffix(frame % vid_opts.segment_size, vid_opts.segment_size) + ".ppm";
                frame_opts.scale = scale;
                save_render(renderer, frame_opts);
            } else {
                wacfrac::logging::debug( "Frame #{} has already been rendered; skipping", frame);
            }
            if (frame != 0 && (frame % vid_opts.segment_size == vid_opts.segment_size - 1
            || static_cast<std::size_t>(frame) == total_frames - 1)) {
                auto status {wacfrac::concatenate_images(
                    segment_filename,
                    "frame_%" + wacfrac::file_suffix_format(vid_opts.segment_size) + ".ppm",
                    static_cast<float>(vid_opts.frames_per_second)
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

template<typename Context>
void dispatch(wacfrac::RendererOptions<Context>& opts, argumentum::CommandOptions* cmd) {
    wacfrac::logging::info("Using {} rendering",
        std::is_same_v<Context, wacfrac::Device> ? "GPU" : "CPU");
    wacfrac::Renderer<Context> renderer {std::move(opts)};
    if (auto* img_opts = dynamic_cast<wacfrac::ImageOptions*>(cmd)) {
        save_render(renderer, *img_opts);
    }
    if (auto* vid_opts = dynamic_cast<wacfrac::VideoOptions*>(cmd)) {
        render_video(renderer, *vid_opts);
    }
}

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
    params.add_command<wacfrac::VideoOptions>("video");

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

    if (has_gpu() && renderer_opts.use_gpu) {
        wacfrac::RendererOptions<wacfrac::Device> dev_opts;
        dev_opts.resolution = renderer_opts.resolution;
        dev_opts.focus = renderer_opts.focus;
        dev_opts.escape_radius = renderer_opts.escape_radius;
        dev_opts.discrete_coloring = renderer_opts.discrete_coloring;
        dev_opts.iteration_parameters = renderer_opts.iteration_parameters; // WARN: It seems like the iteration parameters get flipped. TODO: Make a struct
        dev_opts.bla_config = renderer_opts.bla_config;
        dev_opts.probe_grid = renderer_opts.probe_grid;
        if (renderer_opts.palette.size() > 0) {
            dev_opts.palette = dev_opts.ctx.make_buffer(renderer_opts.palette.as_span());
        }
        dev_opts.log_level = renderer_opts.log_level;
        dispatch(dev_opts, cmd.get());
    } else {
        dispatch(renderer_opts, cmd.get());
    }
}
