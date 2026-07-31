#include "wacfrac/cli_options.hpp"
#include "wacfrac/io.hpp"
#include "wacfrac/log.hpp"
#include "wacfrac/reference.hpp"
#include "wacfrac/viewport.hpp"

#include <chrono>
#include <cstdlib>
#include <print>
#include <string_view>
#include <sycl/sycl.hpp>

namespace
{

void save_render(wacfrac::Renderer& renderer, wacfrac::ImageConfig& img_conf)
{
    if (wacfrac::write_ppm(img_conf.filepath, renderer.conf.resolution, renderer.render(img_conf))) {
        wacfrac::logging::info("Image written to {}", img_conf.filepath);
    } else {
        wacfrac::logging::error("Image write failed");
    }
}

void render_video(wacfrac::Renderer& renderer, wacfrac::VideoConfig& vid_opts)
{
    if (!std::filesystem::create_directory(vid_opts.directory)) {
        wacfrac::logging::error("Directory '{}' failed to create", vid_opts.directory);
        return;
    }
    std::filesystem::current_path(vid_opts.directory);

    auto max_iterations = wacfrac::required_iterations(
        vid_opts.final_scale, renderer.conf.iteration_parameters.modifier, renderer.conf.iteration_parameters.factor,
        renderer.conf.iteration_parameters.exponent);
    auto precision = wacfrac::required_precision(vid_opts.final_scale);
    wacfrac::MultiFloat::default_precision(static_cast<unsigned>(precision));
    wacfrac::MultiComplex::default_precision(static_cast<unsigned>(precision));
    wacfrac::Viewport final_view{renderer.conf.focus, vid_opts.final_scale, renderer.conf.resolution};

    renderer.reserve(static_cast<unsigned>(max_iterations));

    {
        wacfrac::ReferenceSet refs{renderer.conf.queue, max_iterations};
        refs.compute(final_view.center, renderer.conf.escape_radius);
        renderer.cache_references(std::move(refs));
    }

    wacfrac::logging::info("Video pre-compute: final_zoom={} max_iterations={} precision={} ref_size={}",
                           vid_opts.final_scale, max_iterations, precision, renderer.ref_cache.size());
    auto total_frames{wacfrac::total_frames(vid_opts.initial_scale, vid_opts.final_scale, vid_opts.zoom_per_second,
                                            static_cast<float>(vid_opts.frames_per_second))};
    auto total_segments{total_frames / vid_opts.segment_size};
    auto scales{wacfrac::frame_zooms(vid_opts.initial_scale, vid_opts.final_scale, vid_opts.zoom_per_second,
                                     static_cast<float>(vid_opts.frames_per_second))};

    auto start = std::chrono::steady_clock::now();
    for (auto&& [frame, scale] : std::views::enumerate(std::move(scales))) {
        auto segment = frame / vid_opts.segment_size;
        auto frame_filename{"frame_" + wacfrac::file_suffix(frame % vid_opts.segment_size, vid_opts.segment_size) +
                            ".ppm"};
        auto segment_filename{"segment_" + wacfrac::file_suffix(segment, total_segments) + ".mp4"};

        if (!std::filesystem::exists(segment_filename)) {
            if (!std::filesystem::exists(frame_filename)) {
                wacfrac::logging::info("Frame #{}/{} being rendered", frame, total_frames);
                wacfrac::ImageConfig frame_opts;
                frame_opts.filepath =
                    "frame_" + wacfrac::file_suffix(frame % vid_opts.segment_size, vid_opts.segment_size) + ".ppm";
                frame_opts.scale = scale;
                save_render(renderer, frame_opts);

                if (vid_opts.do_preview) {
                    std::filesystem::copy(frame_opts.filepath, "preview.ppm",
                                          std::filesystem::copy_options::overwrite_existing);
                }
            } else {
                wacfrac::logging::debug("Frame #{} has already been rendered; skipping", frame);
            }
            if (frame != 0 && (frame % vid_opts.segment_size == vid_opts.segment_size - 1 ||
                               static_cast<std::size_t>(frame) == total_frames - 1)) {
                auto status{wacfrac::concatenate_images(
                    segment_filename, "frame_%" + wacfrac::file_suffix_format(vid_opts.segment_size) + ".ppm",
                    static_cast<float>(vid_opts.frames_per_second))};
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
            wacfrac::logging::debug("Frame #{} has already been rendered; skipping", frame);
        }
    }

    auto status{wacfrac::concatenate_videos("final.mp4", "segment_*.mp4")};
    if (status) {
        for (auto& entry : std::filesystem::directory_iterator("."))
            if (auto name = entry.path().filename().string(); name.starts_with("segment_") && name.ends_with(".mp4"))
                std::filesystem::remove(entry.path());
        wacfrac::logging::info("Video render complete");
    } else {
        wacfrac::logging::error("Final video failed to compose");
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now() - start);
    wacfrac::logging::info("Video render took {} minutes", elapsed.count());
}

} // namespace
int main(int argc, char* argv[])
{
    wacfrac::logging::init(0);

    auto parse_result = wacfrac::parse_arguments(argc, argv);
    if (!parse_result)
        return EXIT_FAILURE;

    wacfrac::logging::log_level() = parse_result->log_level;

    if (parse_result->mode == wacfrac::CliOptions::Mode::Image) {
        wacfrac::Renderer renderer{std::move(parse_result->renderer)};
        save_render(renderer, parse_result->image);
    } else {
        wacfrac::Renderer renderer{std::move(parse_result->renderer)};
        render_video(renderer, parse_result->video);
    }
}
