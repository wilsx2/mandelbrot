#pragma once
#include "wacfrac/bla.hpp"
#include "wacfrac/buffer.hpp"
#include "wacfrac/color.hpp"
#include "wacfrac/reference.hpp"
#include "wacfrac/resolution.hpp"
#include "wacfrac/types.hpp"
#include "wacfrac/viewport.hpp"

#include <span>
#include <sycl/sycl.hpp>

namespace wacfrac
{

enum class NumericType { Auto, Float, Double, DoubleExp };
enum class RenderType { Auto, Direct, Perturbed, BLA };

struct IterationParameters {
    double modifier{250.0};
    double factor{50.0};
    double exponent{1.5};
};

struct RendererConfig {
    sycl::queue queue{sycl::default_selector_v};
    Resolution resolution{500, 500};
    MultiComplex focus{-0.5, 0.0};
    double escape_radius{4.0};
    DeviceBuffer<Pixel> palette{queue, std::span(wacfrac::ULTRA)};
    bool discrete_coloring{false};
    IterationParameters iteration_parameters{};
    bla::Config bla_config;
    std::pair<std::size_t, std::size_t> probe_grid{64, 64};
};

struct ImageConfig {
    std::string filepath{"mandelbrot.ppm"};
    MultiFloat scale{0.4};
    unsigned max_iterations{0u};
    std::size_t precision{0};
    NumericType numeric_type{NumericType::Auto};
    RenderType render_type{RenderType::Auto};
    double epsilon{0.0};
};

struct Renderer {
public:
    RendererConfig conf; // TODO: make private
    ReferenceSet ref_cache;
    DeviceBuffer<Pixel> pixels;
    DeviceArena arena;

    Renderer(RendererConfig config);
    auto cache_references(ReferenceSet&& refs) -> void;
    auto reserve(unsigned max_n) -> void;
    auto render(ImageConfig img_conf) -> std::span<const Pixel>;

private:
    auto apply_heuristics(ImageConfig& img_conf, Viewport& view) -> void;
    template <typename T, typename F> auto render_pass(T start, T delta, unsigned max_n, F pixel_escape) -> void;
};

} // namespace wacfrac
