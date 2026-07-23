#pragma once
#include "wacfrac/bla.hpp"
#include <cstddef>
#include "wacfrac/reference.hpp"
#include "wacfrac/types.hpp"
#include "wacfrac/resolution.hpp"
#include "wacfrac/color.hpp"
#include <boost/optional.hpp>
#include <span>

namespace wacfrac {

enum class NumericType { Auto , Float , Double , DoubleExp };
enum class RenderType { Auto , Direct , Perturbed , BLA };

struct IterationParameters {
    double modifier {250.0};
    double factor {50.0};
    double exponent {1.5};
};

struct RendererConfig {
    ProcessingContext ctx {};
    Resolution resolution {500, 500};
    MultiComplex focus {-0.5, 0.0};
    double escape_radius {4.0};
    Buffer<Pixel> palette {ctx.make_buffer(std::span(ULTRA))};
    bool discrete_coloring {false};
    IterationParameters iteration_parameters {};
    bla::Config bla_config;
    std::pair<std::size_t, std::size_t> probe_grid {64, 64};
};

struct ImageConfig {
    MultiFloat scale {0.4};
    unsigned max_iterations {0u};
    std::size_t precision {0};
    NumericType numeric_type {NumericType::Auto};
    RenderType render_type {RenderType::Auto};
    double epsilon {0.0};
};

struct Renderer {
    template<typename T>
    using Buffer = wacfrac::Buffer<T>;

    RendererConfig conf;
    ReferenceSet ref_cache;
    Buffer<Pixel> pixels;
    Buffer<DoubleExpComplex> pixel_orbits;
    Buffer<std::byte> arena_buffer;

    Renderer(RendererConfig config);
    auto cache_references(ReferenceSet&& refs) -> void;
    auto render(const ImageConfig& img_conf) -> std::span<const Pixel>;

    template<typename T>
    auto direct_render_pass(T start, T delta, unsigned max_n) -> void;
    template<typename T>
    auto perturbed_render_pass(T start, T delta, std::span<const T> ref, unsigned max_n) -> void;
    template<typename T>
    auto bla_render_pass(T start, T delta, std::span<const T> ref, bla::Approximator<T> bla, unsigned max_n) -> void;
};

}
