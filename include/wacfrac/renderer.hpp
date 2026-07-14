#include "wacfrac/bla.hpp"
#include "wacfrac/types.hpp"
#include "wacfrac/resolution.hpp"
#include "wacfrac/color.hpp"
#include "wacfrac/viewport.hpp"
#include <boost/optional.hpp>
#include <string>

namespace wacfrac {

enum class NumericType { Auto, Float , Double , DoubleExp };
enum class RenderType { Auto, Direct , Perturbed , BLA }; // NOTE: Redefined in cli_options.hpp

struct RendererConfig {
    Resolution resolution {500, 500};
    MultiComplex focus {-0.5, 0.0};
    double escape_radius {4.0};
    std::vector<Pixel> palette {wacfrac::ULTRA};
    bool discrete_coloring {false};
    std::tuple<double, double, double> iteration_parameters {250.0, 50.0, 1.5};
    bla::SearchParams search_params;
    std::size_t bla_first_level {0};
};

struct ImageConfig {
    boost::optional<const ReferenceSet&> ref_set;
    MultiFloat scale {0.4};
    unsigned max_iterations {0u};
    std::size_t precision {0};
    NumericType numeric_type {NumericType::Auto};
    RenderType render_type {RenderType::Auto};
    double epsilon {0.0};
};

struct VideoConfig {
    std::string directory {"mandelbrot"};
    double frames_per_second {24.0};
    std::size_t segment_size {64};
    MultiFloat initial_scale {0.4};
    MultiFloat final_scale {1e1};
    double zoom_per_second {2.0};
};

template<typename Context>
struct Renderer {
    template<typename T>
    using Buffer = Context::template Buffer<T>;
    template<typename T>
    using Pointer = Context::template Pointer<T>;

    Context ctx;
    Resolution resolution;
    MultiComplex focus;
    double escape_radius;
    Buffer<Pixel> palette;
    bool discrete_coloring;
    WF_STD::tuple<double, double, double> iteration_parameters;
    Buffer<Pixel> pixels;
    Buffer<DoubleExpComplex> pixel_orbits;
    bla::GenericCalculator<Context, DoubleExpComplex> dexp_bla_calculator;

    Renderer(const RendererConfig& conf, Context ctx = {})
        : ctx(ctx)
        , resolution(conf.resolution)
        , focus(conf.focus)
        , escape_radius(conf.escape_radius)
        , palette(ctx.copy_buffer(conf.palette))
        , discrete_coloring(conf.discrete_coloring)
        , iteration_parameters(conf.iteration_parameters)
        , pixels(ctx.template make_buffer<Pixel>(resolution.area()))
        , pixel_orbits(ctx.template make_buffer<DoubleExpComplex>(resolution.area()))
        , dexp_bla_calculator(conf.bla_first_level) // TODO: Pass search params. Make it a set?
    {
        // TODO: Log params
    }

    auto render_image(const ImageConfig& conf) -> std::vector<Pixel> {
        auto max_n {conf.max_iterations != 0 
            ? conf.max_iterations
            : required_iterations(
                conf.scale,
                WF_STD::get<0>(iteration_parameters),
                WF_STD::get<1>(iteration_parameters),
                WF_STD::get<2>(iteration_parameters))};
        auto p {conf.precision != 0 
            ? conf.precision
            : required_precision(conf.scale)};
        wacfrac::MultiFloat::default_precision(static_cast<unsigned>(p));
        wacfrac::MultiComplex::default_precision(static_cast<unsigned>(p));

        /* T delta {
            to_real<CT>(view.dimensions.real()) / static_cast<CT>(resolution.width),
            to_real<CT>(view.dimensions.imag()) / static_cast<CT>(resolution.height)
        }; */
        auto render_type {[&](){
            if (conf.render_type != RenderType::Auto) {
                return conf.render_type;
            }
            // compute delta of float for direct perhaps? maybe just always do BLA or perturbed
            auto 
            if (delta == 0.0) {
                //if (/*max_iters > magic num*/) {
                    return RenderType::BLA;
                //}
                //return RenderType::Perturbed;
            }
            return RenderType::Direct;
        }};
        auto num_type {conf.numeric_type != NumericType::Auto
            ? conf.numeric_type
            : NumericType::Float};

        // TODO: Auto-determine ideal numeric and render type

        wacfrac::logging::info(
            "Render: zoom={} max_iterations={} precision={} numeric_type={} render_type={}",
            focus.real(), focus.imag(), conf.scale,
            max_n, p, num_type, [&](){
                switch (render_type) {
                    case wacfrac::RenderType::Direct: return "direct";
                    case wacfrac::RenderType::Perturbed: return "perturbed";
                    case wacfrac::RenderType::BLA: return "bla";
                    case wacfrac::RenderType::Auto: return "ERROR";
                }
                return "???";
            }());

        auto start = std::chrono::steady_clock::now();
        with_numeric_type(num_type, [&]<typename T>(NumericTypeTag<T>){
            Viewport v {focus, conf.scale, resolution};
            v.precision(p);
            auto orbits {pixel_orbits.as_span<T>()}; // NOTE: as_span is not currently a template

            if (render_type == wacfrac::RenderType::Direct) {
                // TODO: parallel for; render
            } else {
                Viewport view {focus, conf.scale, resolution};
                view.precision(p);
                auto c_ref {focus};
                const auto& ref {
                    conf.ref_set.has_value()
                    ? conf.ref_set->select<T>()
                    : wacfrac::compute_reference_mt<T>(
                        c_ref, max_n, 
                        std::numeric_limits<double>::infinity())
                };

                if (render_type == wacfrac::RenderType::Perturbed) {
                    // TODO: parallel for; render
                } else if (render_type == wacfrac::RenderType::BLA) {
                    using CT = wacfrac::ComplexValueTypeT<T>;
                    auto max_dc {wacfrac::to_complex<T>(view.compute_max_dc(c_ref))};
                    if (conf.epsilon != 0.0) {
                        dexp_bla_calculator.compute_manual(static_cast<CT>(conf.epsilon), ref, max_dc);
                    } else {
                        dexp_bla_calculator.compute_search( // NOTE: Assumes search params were initialized in the BLA
                            view.generate_probes<T>(probe_grid.first, probe_grid.second),
                            max_dc, ref, escape_radius);
                    }
                    auto bla = dexp_bla_calculator.get_approximator();

                    // TODO: parallel for; render
                }
            }
        });
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start
        );

        logging::info("Image render took {}ms", elapsed.count());
        return pixels;
    }
};

}
