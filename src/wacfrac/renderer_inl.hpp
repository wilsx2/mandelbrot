#pragma once
#include "wacfrac/bla.hpp"
#include "wacfrac/renderer.hpp"
#include "wacfrac/complex_concept.hpp"
#include "wacfrac/orbit.hpp"
#include "wacfrac/rendering.hpp"
#include "wacfrac/viewport.hpp"

namespace wacfrac {

struct Colorizer {
    bool discrete;
    WF_STD::span<const Pixel> palette;
    unsigned max_n;

    template<typename Z, typename N>
    WF_HD
    auto colorize(Z z, N n) const -> Pixel {
        if (discrete)
            return colorize_discrete(n, max_n, palette);
        return colorize_continuous(z, n, max_n, palette);
    }
};

template <typename T>
struct NumericTypeTag { using type = T; };

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
template<typename T>
auto Renderer<Context>::direct_render_pass(T start, T delta, unsigned max_n) -> void {
    auto discrete {conf.discrete_coloring};
    auto palette {conf.palette.as_span()};
    auto screen {pixels.as_span()};
    auto row_width {conf.resolution.width};
    auto escape_radius {conf.escape_radius};

    Colorizer colorize {discrete, palette, max_n};
    conf.ctx.parallel_for(screen.size(),
        [screen,
        row_width,
        start,
        delta,
        max_n,
        escape_radius,
        colorize]
        WF_HD
        (int tid) -> void {
            auto [z, n] = escape(
                sample_c_value(
                    tid,
                    row_width,
                    start,
                    delta),
                max_n,
                escape_radius);
            screen[tid] = colorize.colorize(z, n);
        });
}
template<typename Context>
template<typename T>
auto Renderer<Context>::perturbed_render_pass(T start, T delta, WF_STD::span<const T> ref, unsigned max_n) -> void {
    auto discrete {conf.discrete_coloring};
    auto palette {conf.palette.as_span()};
    auto screen {pixels.as_span()};
    auto row_width {conf.resolution.width};
    auto escape_radius {conf.escape_radius};

    Colorizer colorize {discrete, palette, max_n};
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
        (int tid) -> void {
            auto [z, n] = escape_perturbed(
                sample_c_value(
                    tid,
                    row_width,
                    start,
                    delta),
                ref,
                max_n,
                escape_radius);
            screen[tid] = colorize.colorize(z, n);
        });
}
template<typename Context>
template<typename T>
auto Renderer<Context>::bla_render_pass(T start, T delta, WF_STD::span<const T> ref, bla::Approximator<T> bla, unsigned max_n) -> void {
    auto discrete {conf.discrete_coloring};
    auto palette {conf.palette.as_span()};
    auto screen {pixels.as_span()};
    auto row_width {conf.resolution.width};
    auto escape_radius {conf.escape_radius};
    Colorizer colorize {discrete, palette, max_n};
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
        (int tid) -> void {
            auto [z, n, _skipped] = bla::escape_approximate(
                sample_c_value(
                    tid,
                    row_width,
                    start,
                    delta),
                ref,
                max_n,
                escape_radius,
                bla);
            screen[tid] = colorize.colorize(z, n);
        });

}

constexpr auto A_GIGABYTE {1'000'000'000ull};
constexpr auto ARENA_SIZE {A_GIGABYTE};
template <typename Context>
Renderer<Context>::Renderer(RendererConfig<Context> config)
    : conf(std::move(config))
    , pixels(this->conf.ctx.template make_buffer<Pixel>(this->conf.resolution.area()))
    , arena_buffer(this->conf.ctx.template make_buffer<std::byte>(ARENA_SIZE))
{
    if (conf.palette.size() == 0) {
        conf.palette = conf.ctx.make_buffer(WF_STD::span(ULTRA));
    }

    // NOTE: Jesus Christ!
    logging::info("Renderer Config: resolution={}x{} focus={}x{} escape_radius={} palette size={}\
                    discrete={} iteration_params={} + {} * exp^{} first_bla_level={},\
                    epsilon_range=10^{}-10^{} epsilon_tolerance={} epsilon_convergence_rad={}",
                    conf.resolution.width, conf.resolution.height, conf.focus.real(), conf.focus.imag(),
                    conf.escape_radius, conf.palette.as_span().size(), conf.discrete_coloring,
                    conf.iteration_parameters.modifier, conf.iteration_parameters.factor,
                    conf.iteration_parameters.exponent, conf.bla_config.first_level, 
                    conf.bla_config.lower_exp, conf.bla_config.upper_exp, conf.bla_config.tolerance,
                    conf.bla_config.convergence_radius);
}

template<typename Context>
 auto Renderer<Context>::cache_references(ReferenceSet<Context>&& refs) -> void {
    ref_cache = std::move(refs);
}

template<typename Context>
auto Renderer<Context>::render(const ImageConfig& img_conf) -> std::span<const Pixel> {
    // Configuration Pass  NOTE: We can easily and likely ought to break this into its own function
    auto max_n {img_conf.max_iterations != 0 
        ? img_conf.max_iterations
        : required_iterations(
            img_conf.scale,
            conf.iteration_parameters.modifier,
            conf.iteration_parameters.factor,
            conf.iteration_parameters.exponent)};
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
        return start.real() - next.real() == 0 || start.imag() - next.imag() == 0; // NOTE: May need a larger tolerance
    }};

    auto render_type {[&](){
        if (img_conf.render_type != RenderType::Auto) {
            return img_conf.render_type;
        }

        if (underflows.template operator()<Complex<double>>(RenderType::Direct)) {
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

        auto delta {get_pixel_delta<T>(view.dimensions, conf.resolution)};
        logging::debug("delta c: ({}, {})", static_cast<double>(delta.real()), static_cast<double>(delta.imag()));

        if (render_type == RenderType::Direct) {
            auto start {view.get_corner_absolute<T>()};
            logging::debug("start, absolute: ({}, {})", static_cast<double>(start.real()), static_cast<double>(start.imag()));

            direct_render_pass(start, delta, max_n);
        } else {
            Arena arena {arena_buffer.as_span()};

            auto start {view.get_corner_relative<T>()};
            logging::debug("start, relative: ({}, {})", static_cast<double>(start.real()), static_cast<double>(start.imag()));

            auto c_ref {conf.focus};
            WF_STD::span<const T> ref {[&](){
                if (ref_cache.max_n() >= max_n)
                    return ref_cache.template select<T>();
                auto buf {arena.alloc<T>(max_n)}; // NOTE: We may be able to give some memory back
                auto n {compute_reference_mt<T>(buf, c_ref, conf.escape_radius)}; 
                return buf.subspan(0, n);
            }()};

            if (render_type == RenderType::Perturbed) {
                perturbed_render_pass(start, delta, ref, max_n);
            } else if (render_type == RenderType::BLA) {
                using CT = ComplexValueTypeT<T>;
                auto max_dc {to_complex<T>(view.compute_max_dc(c_ref))};
                bla::Calculator<Context, T> bla_calculator {conf.bla_config}; // WARN: Filthy Nasty. Throw this an an Arena
                if (img_conf.epsilon != 0.0) {
                    bla_calculator.compute_manual(static_cast<CT>(img_conf.epsilon), ref, max_dc);
                } else {
                    auto probes {conf.ctx.template make_buffer<T>(conf.probe_grid.first * conf.probe_grid.second)};
                        // WARN: FILTH! NAST! IN GODS NAME FORSAKE THIS WRETCHED ALLOCATION!
                        // WARN: I'm going to be sick.
                    view.generate_probes<T>(conf.ctx, probes.as_span(), conf.probe_grid.first, conf.probe_grid.second);
                    bla_calculator.compute_search(probes, max_dc, ref, conf.escape_radius);
                }
                auto bla = bla_calculator.get_approximator();
                bla_render_pass(start, delta, ref, bla, max_n);
            }
        }
    });
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>( // NOTE: Unnecessary if log level > info
        std::chrono::steady_clock::now() - start
    );

    logging::info("Image render took {}ms", elapsed.count());
    return pixels.as_span();
}

}
