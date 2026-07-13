#include "wacfrac/bla.hpp"
#include "wacfrac/types.hpp"
#include "wacfrac/resolution.hpp"
#include "wacfrac/color.hpp"
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

template<typename Derived>
class Renderer {
    Resolution resolution;
    MultiComplex focus;
    double escape_radius;
    /* CRTP storage: palette */
    bool discrete_coloring;
    std::tuple<double, double, double> iteration_parameters;
    bla::SearchParams search_params;
    std::size_t bla_first_level;
    std::vector<wacfrac::Pixel> pixels;

    Renderer(const RendererConfig& conf)
        : resolution(conf.resolution)
        , focus(conf.focus)
        , escape_radius(conf.escape_radius)
        // "palette" is initialized in derived storage
        , discrete_coloring(conf.discrete_coloring)
        , iteration_parameters(conf.iteration_parameters)
        , search_params(conf.search_params)
        , pixels((resolution.area()))
    {
        // TODO: Log params
    }

    auto render_image(const ImageConfig& conf) -> std::vector<Pixel> {
        auto max_n          {conf.effective_max_iterations()};
        auto num_type       {conf.effective_numeric_type()};
        auto render_type    {conf.effective_render_type()};
        auto p              {conf.effective_precision()};
        wacfrac::logging::info(
            "Render: zoom={} max_iterations={} precision={} numeric_type={} render_type={}",
            focus.real(), focus.imag(), conf.scale,
            max_n, p, num_type, [&](){
                switch (render_type) {
                    case wacfrac::RenderType::Direct: return "direct";
                    case wacfrac::RenderType::Perturbed: return "perturbed";
                    case wacfrac::RenderType::BLA: return "bla";
                }
                return "???";
            }());

        wacfrac::MultiFloat::default_precision(static_cast<unsigned>(p));
        wacfrac::MultiComplex::default_precision(static_cast<unsigned>(p));


        auto start = std::chrono::steady_clock::now();
        // TODO: Dynamically select render type and numeric type based on measurements
        with_numeric_type(num_type, [&]<typename T>(NumericTypeTag<T>){
            std::vector<std::pair<wacfrac::Complex<float>, unsigned>> escaped_orbits;
            escaped_orbits.reserve(pixels.size());

            if (render_type == wacfrac::RenderType::Direct) {
                Viewport v {focus, conf.scale, resolution};
                v.precision(p);
                auto cs {wacfrac::sample_c_values<T>(v, resolution)};
                for (auto c : cs)
                    escaped_orbits.push_back(wacfrac::escape(c, max_n, escape_radius));
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
                auto dcs {wacfrac::sample_c_values<T>(
                    view, resolution,
                    wacfrac::to_complex<T>(c_ref)
                )};

                if (render_type == wacfrac::RenderType::Perturbed) {
                    // TODO: CRTP
                    for (auto dc : dcs)
                        escaped_orbits.push_back(wacfrac::escape_perturbed(dc, std::span<const T>(ref), max_n, escape_radius));
                    // END
                } else if (render_type == wacfrac::RenderType::BLA) {
                    using CT = wacfrac::ComplexValueTypeT<T>;
                    auto max_dc {wacfrac::to_complex<T>(view.compute_max_dc(c_ref))};
                    // TODO: CRTP
                    wacfrac::bla::Calculator<T> calculator{first_level};
                    if (conf.epsilon != 0.0) {
                        calculator.compute_manual(static_cast<CT>(conf.epsilon), ref, max_dc);
                    } else {
                        calculator.compute_search(search_params,
                            view.generate_probes<T>(probe_grid.first, probe_grid.second),
                            max_dc, ref, escape_radius);
                    }
                    auto bla = calculator.get_approximator();
                    // END
                    // TODO: CRTP
                    for (auto dc : dcs) {
                        auto [z, n, _] = wacfrac::bla::escape_approximate(dc, std::span<const T>(ref), static_cast<unsigned>(ref.size()), escape_radius, bla);
                        escaped_orbits.emplace_back(z, n);
                    }
                    // END
                }
            }

            // TODO: CRTP
            for (auto&& [pixel, orbit] : std::views::zip(pixels, escaped_orbits)) {
                pixel = discrete_coloring
                    ? wacfrac::colorize_discrete(std::get<1>(orbit), max_n, palette)
                    : wacfrac::colorize_continuous(wacfrac::Complex<float>{std::get<0>(orbit).real(), std::get<0>(orbit).imag()}, std::get<1>(orbit), max_n, palette);
            } 
            // END
        });
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start
        );

        logging::info("Image render took {}ms", elapsed.count());
        return pixels;
    }
};

}
