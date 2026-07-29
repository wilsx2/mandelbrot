#include "wacfrac/bla.hpp"
#include "wacfrac/renderer.hpp"
#include "wacfrac/complex_concept.hpp"
#include "wacfrac/orbit.hpp"
#include "wacfrac/rendering.hpp"
#include "wacfrac/viewport.hpp"
#include <sycl/access/access.hpp>
#include <sycl/accessor.hpp>

namespace wacfrac {

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

template<typename T>
auto compute_arena_size(unsigned max_n, RenderType render_type, const bla::Config& bla_config,
                        std::pair<std::size_t, std::size_t> probe_grid) -> std::size_t {
    using BlaT = bla::Bla<T>;
    auto probe_count = probe_grid.first * probe_grid.second;

    if (render_type == RenderType::Direct)
        return 0;

    std::size_t bytes = sizeof(T) * max_n;

    if (render_type == RenderType::BLA) {
        auto last_level = max_n < 3 ? std::size_t{0} : static_cast<std::size_t>(std::log2(static_cast<double>(max_n)));
        auto n = max_n - 1;
        auto approx_count = bla::ColumnInfo::compute_start(n, bla_config.first_level, last_level);

        bytes += sizeof(T) * probe_count;
        bytes += sizeof(bla::ColumnInfo) * n;
        bytes += sizeof(BlaT) * (n - 1) * 2;
        bytes += sizeof(BlaT) * approx_count;
        bytes += sizeof(unsigned) * (probe_count + 2);
    }

    return bytes;
}

template<typename T>
auto Renderer::direct_render_pass(T start, T delta, unsigned max_n) -> void {
    auto discrete {conf.discrete_coloring};
    auto escape_radius {conf.escape_radius};
    auto cols {conf.resolution.width};
    auto screen_span {pixels.as_span()};
    auto palette_span {conf.palette.as_span()};

    conf.queue.parallel_for(conf.resolution.range(), [=](sycl::id<2> id) {
        auto [z, n] = escape(
            sample_c_value(id, start, delta),
            max_n,
            escape_radius);
        screen_span[id.get(1) * cols + id.get(0)] = [&](){
            if (discrete) [[unlikely]]
                return colorize_discrete(n, max_n, palette_span);
            return colorize_continuous(z, n, max_n, palette_span);
        }();
    }).wait();
}
template<typename T>
auto Renderer::perturbed_render_pass(T start, T delta, std::span<const T> ref, unsigned max_n) -> void {
    auto discrete {conf.discrete_coloring};
    auto escape_radius {conf.escape_radius};
    auto cols {conf.resolution.width};
    auto screen_span {pixels.as_span()};
    auto palette_span {conf.palette.as_span()};

    conf.queue.parallel_for(conf.resolution.range(), [=](sycl::id<2> id) {
        auto [z, n] = escape_perturbed(
            sample_c_value(id, start, delta),
            ref,
            max_n,
            escape_radius);
        screen_span[id.get(1) * cols + id.get(0)] = [&](){
            if (discrete) [[unlikely]]
                return colorize_discrete(n, max_n, palette_span);
            return colorize_continuous(z, n, max_n, palette_span);
        }();
    }).wait();
}
template<typename T>
auto Renderer::bla_render_pass(T start, T delta, std::span<const T> ref, bla::Approximator<T> bla, unsigned max_n) -> void {
    auto discrete {conf.discrete_coloring};
    auto escape_radius {conf.escape_radius};
    auto cols {conf.resolution.width};
    auto screen_span {pixels.as_span()};
    auto palette_span {conf.palette.as_span()};

    conf.queue.parallel_for(conf.resolution.range(), [=](sycl::id<2> id) {
        auto [z, n, _] = bla::escape_approximate(
            sample_c_value(id, start, delta),
            ref,
            max_n,
            escape_radius,
            bla);
        screen_span[id.get(1) * cols + id.get(0)] = [&](){
            if (discrete) [[unlikely]]
                return colorize_discrete(n, max_n, palette_span);
            return colorize_continuous(z, n, max_n, palette_span);
        }();
    }).wait();
}

Renderer::Renderer(RendererConfig config)
    : conf {std::move(config)}
    , pixels {conf.queue, conf.resolution.area()}
    , arena {conf.queue, 0}
{
    if (conf.palette.size() == 0) {
        conf.palette = {conf.queue, std::span(ULTRA)};
    }

    auto hint_max_n = required_iterations(
        MultiFloat(10.0),
        conf.iteration_parameters.modifier,
        conf.iteration_parameters.factor,
        conf.iteration_parameters.exponent);
    auto initial_size = compute_arena_size<DoubleExpComplex>(
        static_cast<unsigned>(hint_max_n), RenderType::BLA, conf.bla_config, conf.probe_grid);
    if (initial_size > 0)
        arena.grow(initial_size);

    logging::info("Renderer Config: device={} resolution={}x{} focus={}x{} escape_radius={} palette size={}\
                    discrete={} iteration_params={} + {} * exp^{} first_bla_level={},\
                    epsilon_range=10^{}-10^{} epsilon_tolerance={} epsilon_convergence_rad={}",
                    conf.queue.get_device().get_info<sycl::info::device::name>(),
                    conf.resolution.width, conf.resolution.height, conf.focus.real(), conf.focus.imag(),
                    conf.escape_radius, conf.palette.as_span().size(), conf.discrete_coloring,
                    conf.iteration_parameters.modifier, conf.iteration_parameters.factor,
                    conf.iteration_parameters.exponent, conf.bla_config.first_level,
                    conf.bla_config.lower_exp, conf.bla_config.upper_exp, conf.bla_config.tolerance,
                    conf.bla_config.convergence_radius);
}

auto Renderer::cache_references(ReferenceSet&& refs) -> void {
    ref_cache = std::move(refs);
}

auto Renderer::reserve(unsigned max_n) -> void {
    auto needed = compute_arena_size<DoubleExpComplex>(max_n, RenderType::BLA, conf.bla_config, conf.probe_grid);
    arena.grow(needed);
}

auto Renderer::render(const ImageConfig& img_conf) -> std::span<const Pixel> {
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
        using CT = ComplexValueTypeT<T>;
        auto start {[&](){
            if (rt == RenderType::Direct) {
                return view.get_corner_absolute<T>();
            }
            return view.get_corner_relative<T>();
        }()};
        auto delta {get_pixel_delta<T>(view.dimensions, conf.resolution)};
        volatile CT next_real {static_cast<CT>(start.real() + delta.real())};
        volatile CT next_imag {static_cast<CT>(start.imag() + delta.imag())};
        return start.real() - next_real == 0.0 || start.imag() - next_imag == 0.0;
    }};

    auto render_type {[&](){
        if (img_conf.render_type != RenderType::Auto) {
            return img_conf.render_type;
        }

        if (underflows.template operator()<Complex<float>>(RenderType::Direct)) {
            constexpr auto SIGNIFICANT_ITERATIONS {50'000};
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

    auto start = std::chrono::steady_clock::now();
    with_numeric_type(num_type, [&]<typename T>(NumericTypeTag<T>){
        using CT = ComplexValueTypeT<T>;

        auto needed = compute_arena_size<T>(max_n, render_type, conf.bla_config, conf.probe_grid);
        arena.grow(needed);

        auto delta {get_pixel_delta<T>(view.dimensions, conf.resolution)};
        logging::debug("delta c: ({}, {})", static_cast<double>(delta.real()), static_cast<double>(delta.imag()));

        if (render_type == RenderType::Direct) {
            auto start {view.get_corner_absolute<T>()};
            logging::debug("start, absolute: ({}, {})", static_cast<double>(start.real()), static_cast<double>(start.imag()));

            direct_render_pass(start, delta, max_n);
        } else {
            auto start {view.get_corner_relative<T>()};
            logging::debug("start, relative: ({}, {})", static_cast<double>(start.real()), static_cast<double>(start.imag()));

            auto c_ref {conf.focus};
            std::span<const T> ref {[&](){
                if (ref_cache.max_n() >= max_n)
                    return ref_cache.template select<T>();
                auto buf {arena.allocate<T>(max_n)};
                auto n {compute_reference_mt<T>(buf, c_ref, conf.escape_radius)};
                return buf.subspan(0, n);
            }()};

            if (render_type == RenderType::Perturbed) {
                perturbed_render_pass(start, delta, ref, max_n);
            } else if (render_type == RenderType::BLA) {
                using CT = ComplexValueTypeT<T>;
                auto max_dc {to_complex<T>(view.compute_max_dc(c_ref))};
                bla::Calculator<T> bla_calculator {conf.queue, arena, conf.bla_config};
                if (img_conf.epsilon != 0.0) {
                    bla_calculator.compute_manual(static_cast<CT>(img_conf.epsilon), ref, max_dc);
                } else {
                    auto probes {arena.allocate<T>(conf.probe_grid.first * conf.probe_grid.second)};
                    view.generate_probes<T>(conf.queue, probes, sycl::range(conf.probe_grid.first, conf.probe_grid.second));
                    bla_calculator.compute_search(probes, max_dc, ref, conf.escape_radius);
                }
                auto bla = bla_calculator.get_approximator();
                bla_render_pass(start, delta, ref, bla, max_n);
            }

            arena.reset();
        }
    });
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start
    );

    conf.queue.wait();
    logging::info("Image render took {}ms", elapsed.count());
    return pixels.as_span();
}

}
