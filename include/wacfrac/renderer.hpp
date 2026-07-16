#include "wacfrac/bla.hpp"
#include "wacfrac/types.hpp"
#include "wacfrac/resolution.hpp"
#include "wacfrac/color.hpp"
#include <boost/optional.hpp>

namespace wacfrac {

enum class NumericType { Auto , Float , Double , DoubleExp };
enum class RenderType { Auto , Direct , Perturbed , BLA };

template<typename Context>
struct RendererConfig {
    Context ctx {};
    Resolution resolution {500, 500};
    MultiComplex focus {-0.5, 0.0};
    double escape_radius {4.0};
    Context::template Buffer<Pixel> palette {ctx.make_buffer(WF_STD::span(ULTRA))};
    bool discrete_coloring {false};
    WF_STD::tuple<double, double, double> iteration_parameters {1.5, 50.0, 250.0};
    bla::Config bla_config;
    WF_STD::pair<std::size_t, std::size_t> probe_grid {8, 8};
};

struct ImageConfig {
    boost::optional<const ReferenceSet&> ref_set; // NOTE: Should belong to the renderer, not the image
    MultiFloat scale {0.4};
    unsigned max_iterations {0u};
    std::size_t precision {0};
    NumericType numeric_type {NumericType::Auto};
    RenderType render_type {RenderType::Auto};
    double epsilon {0.0};
};

template<typename Context>
struct Renderer {
    template<typename T>
    using Buffer = Context::template Buffer<T>;
    template<typename T>
    using Pointer = Context::template Pointer<T>;

    RendererConfig<Context> conf;
    Buffer<Pixel> pixels;
    Buffer<DoubleExpComplex> pixel_orbits;

    Renderer(RendererConfig<Context> config);
    auto render(const ImageConfig& img_conf) -> WF_STD::span<const Pixel>;

    template<typename T>
    auto direct_render_pass(T start, T delta, unsigned max_n) -> void;
    template<typename T>
    auto perturbed_render_pass(T start, T delta, WF_STD::span<const T> ref, unsigned max_n) -> void;
    template<typename T>
    auto bla_render_pass(T start, T delta, WF_STD::span<const T> ref, bla::Approximator<T> bla, unsigned max_n) -> void;
};

}
