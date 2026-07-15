#include "wacfrac/bla.hpp"
#include "wacfrac/complex_concept.hpp"
#include "wacfrac/orbit.hpp"
#include "wacfrac/rendering.hpp"
#include "wacfrac/types.hpp"
#include "wacfrac/resolution.hpp"
#include "wacfrac/color.hpp"
#include "wacfrac/viewport.hpp"
#include <boost/optional.hpp>
#include <string>

namespace wacfrac {

template <typename T>
struct NumericTypeTag { using type = T; };

enum class NumericType { Auto, Float , Double , DoubleExp };
enum class RenderType { Auto, Direct , Perturbed , BLA };

template <typename F>
decltype(auto) with_numeric_type(NumericType type, F&& f) {
    switch (type) {
        case NumericType::Float:     return f(NumericTypeTag<wacfrac::SingleComplex>{});
        case NumericType::Double:    return f(NumericTypeTag<wacfrac::DoubleComplex>{});
        case NumericType::DoubleExp: return f(NumericTypeTag<wacfrac::DoubleExpComplex>{});
        case NumericType::Auto:      return f(NumericTypeTag<wacfrac::SingleComplex>{});
    }
}

template<typename Context>
struct RendererConfig {
    Context ctx {};
    Resolution resolution {500, 500};
    MultiComplex focus {-0.5, 0.0};
    double escape_radius {4.0};
    Context::template Buffer<Pixel> palette {ctx.make_buffer(std::span(ULTRA))};
    bool discrete_coloring {false};
    WF_STD::tuple<double, double, double> iteration_parameters {250.0, 50.0, 1.5};
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

struct VideoConfig {
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

    RendererConfig<Context> conf;
    Buffer<Pixel> pixels;
    Buffer<DoubleExpComplex> pixel_orbits;

    Renderer(RendererConfig<Context> config)
        : conf(std::move(config))
        , pixels(this->conf.ctx.template make_buffer<Pixel>(this->conf.resolution.area()))
    {
        if (conf.palette.size() == 0) {
            conf.palette = conf.ctx.make_buffer(std::span(ULTRA));
        }

        // NOTE: Jesus Christ!
        logging::info("Renderer Config: resolution={}x{} focus={}x{} escape_radius={} palette size={}\
                       discrete={} iteration_params={} + {} * exp^{} first_bla_level={},\
                       epsilon_range=10^{}-10^{} epsilon_tolerance={} epsilon_convergence_rad={}",
                       conf.resolution.width, conf.resolution.height, conf.focus.real(), conf.focus.imag(),
                       conf.escape_radius, conf.palette.as_span().size(), conf.discrete_coloring,
                       WF_STD::get<0>(conf.iteration_parameters), WF_STD::get<1>(conf.iteration_parameters),
                       WF_STD::get<2>(conf.iteration_parameters), conf.bla_config.first_level, 
                       conf.bla_config.lower_exp, conf.bla_config.upper_exp, conf.bla_config.tolerance,
                       conf.bla_config.convergence_radius);
    }

    auto render(const ImageConfig& img_conf) -> Buffer<Pixel> {
        // Configuration Pass  NOTE: We can easily and likely ought to break this into its own function
        auto max_n {img_conf.max_iterations != 0 
            ? img_conf.max_iterations
            : required_iterations(
                img_conf.scale,
                WF_STD::get<0>(conf.iteration_parameters),
                WF_STD::get<1>(conf.iteration_parameters),
                WF_STD::get<2>(conf.iteration_parameters))};
        auto precision {img_conf.precision != 0 
            ? img_conf.precision
            : required_precision(img_conf.scale)};
        MultiFloat::default_precision(static_cast<unsigned>(precision));
        MultiComplex::default_precision(static_cast<unsigned>(precision));

        Viewport view {conf.focus, img_conf.scale, conf.resolution};
        view.precision(precision);

        auto underflows {[&]<typename T>(RenderType rt){
            auto start {[&](){
                if (rt == RenderType::Direct) {
                    return view.get_corner_absolute<T>();
                }
                return view.get_corner_relative<T>();
            }()};
            auto delta {get_pixel_delta<T>(view.dimensions, conf.resolution)};
            auto next {start + delta};
            return start.real() - next.real() == 0 || start.imag() - next.imag() == 0;
        }};

        auto render_type {[&](){
            if (img_conf.render_type != RenderType::Auto) {
                return img_conf.render_type;
            }
            if (underflows.template operator()<Complex<double>>(RenderType::Direct)) { // NOTE: May need a larger tolerance
                constexpr auto SIGNIFICANT_ITERATIONS {50'000}; // NOTE: Arbitrarily chosen
                if (max_n >= SIGNIFICANT_ITERATIONS) {
                    return RenderType::BLA;
                }
                return RenderType::Perturbed;
            }
            return RenderType::Direct;
        }()};

        auto num_type {[&](){
            if (img_conf.numeric_type != NumericType::Auto) {
                return img_conf.numeric_type;
            }

            if (underflows.template operator()<Complex<float>>(render_type)) {
                if (underflows.template operator()<Complex<double>>(render_type)) {
                    return NumericType::DoubleExp;
                }
                return NumericType::Double;
            }
            return NumericType::Float;
        }()};

        logging::info(
            "Render Config: zoom={} max_iterations={} precision={} numeric_type={} render_type={}",
            img_conf.scale, max_n, precision,
            [&](){
                switch (num_type) {
                    case NumericType::Float: return "float";
                    case NumericType::Double: return "double";
                    case NumericType::DoubleExp: return "dexp";
                    case NumericType::Auto: return "ERROR";
                }
                return "???";
            }(),
            [&](){
                switch (render_type) {
                    case RenderType::Direct: return "direct";
                    case RenderType::Perturbed: return "perturbed";
                    case RenderType::BLA: return "bla";
                    case RenderType::Auto: return "ERROR";
                }
                return "???";
            }());

        // Render Pass
        auto start = std::chrono::steady_clock::now(); // NOTE: Unnecessary if log level > info
        with_numeric_type(num_type, [&]<typename T>(NumericTypeTag<T>){
            using CT = ComplexValueTypeT<T>;

            auto palette {conf.palette.as_span()};
            auto discrete {conf.discrete_coloring};
            auto colorize {[discrete, palette, max_n](auto orbit){ // WARN: Per-pixel branch we know the result of in advance
                    auto z {std::get<0>(orbit)};
                    auto n {std::get<1>(orbit)};

                    if (discrete)
                        return colorize_discrete(n, max_n, palette);
                    return colorize_continuous(z, n, max_n, palette);
                }};

            auto screen {pixels.as_span()};
            auto row_width {conf.resolution.width};
            auto escape_radius {conf.escape_radius};
            auto delta {get_pixel_delta<T>(view.dimensions, conf.resolution)};
            logging::debug("delta c: ({}, {})", delta.real(), delta.imag());

            if (render_type == RenderType::Direct) {
                auto start {view.get_corner_absolute<T>()};
                logging::debug("start, absolute: ({}, {})", start.real(), start.imag());

                conf.ctx.parallel_for(screen.size(),
                    [screen,
                     row_width,
                     start,
                     delta,
                     max_n,
                     escape_radius,
                     colorize]
                    WF_HD
                    (int tid){
                        screen[tid] = colorize(escape(
                            sample_c_value(
                                tid,
                                row_width,
                                start,
                                delta),
                            max_n,
                            escape_radius));
                    });
            } else {
                auto start {view.get_corner_relative<T>()};
                logging::debug("start, relative: ({}, {})", start.real(), start.imag());

                auto c_ref {conf.focus};
                const auto& reference {
                    img_conf.ref_set.has_value()
                    ? img_conf.ref_set->select<T>()
                    : compute_reference_mt<T>(
                        c_ref, max_n, 
                        std::numeric_limits<double>::infinity())
                };
                auto ref {std::span(reference)}; // NOTE: When reference becomes a buffer type we will have to change this 

                if (render_type == RenderType::Perturbed) {
                    conf.ctx.parallel_for(screen.size(),
                        [screen,
                        row_width,
                        start,
                        delta,
                        ref,
                        max_n,
                        escape_radius,
                        colorize]
                        WF_HD
                        (int tid){
                            screen[tid] = colorize(escape_perturbed(
                                sample_c_value(
                                    tid,
                                    row_width,
                                    start,
                                    delta),
                                ref,
                                max_n,
                                escape_radius));
                        });
                } else if (render_type == RenderType::BLA) {
                    using CT = ComplexValueTypeT<T>;
                    auto max_dc {to_complex<T>(view.compute_max_dc(c_ref))};
                    bla::Calculator<Context, T> bla_calculator {conf.bla_config}; // WARN: Filthy Nasty. Throw this an an Arena
                    if (img_conf.epsilon != 0.0) {
                        bla_calculator.compute_manual(static_cast<CT>(img_conf.epsilon), reference, max_dc);
                    } else {
                        bla_calculator.compute_search(
                            view.generate_probes<T>(conf.probe_grid.first, conf.probe_grid.second),
                            max_dc, reference, conf.escape_radius);
                    }
                    auto bla = bla_calculator.get_approximator();
                    conf.ctx.parallel_for(screen.size(),
                        [screen,
                        row_width,
                        start,
                        delta,
                        ref,
                        max_n,
                        escape_radius,
                        colorize,
                        bla]
                        WF_HD
                        (int tid){
                            screen[tid] = colorize(bla::escape_approximate(
                                sample_c_value(
                                    tid,
                                    row_width,
                                    start,
                                    delta),
                                ref,
                                max_n,
                                escape_radius,
                                bla));
                        });
                }
            }
        });
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>( // NOTE: Unnecessary if log level > info
            std::chrono::steady_clock::now() - start
        );

        logging::info("Image render took {}ms", elapsed.count());
        return std::move(pixels);
    }
};

}
