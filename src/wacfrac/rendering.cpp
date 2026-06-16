#include <wacfrac/rendering.hpp>
#include <wacfrac/approximation.hpp>

namespace wacfrac
{

template <std::invocable<std::size_t, std::size_t> F, std::invocable<std::size_t> C>
static auto render_generic(const resolution& res, const std::span<pixel>& buffer, F&& escape_fn, C&& color_fn) -> bool {
    if (res.area() != buffer.size())
        return false;
    auto i {0uz};
    for (auto [y, x] : res.coordinates()) {
        buffer[i++] = color_fn(escape_fn(x, y));
    }
    return true;
}

static auto render_direct(const render_config& conf, const std::span<pixel>& buffer) -> bool {
    return render_generic(conf.res, buffer,
        [&](std::size_t x, std::size_t y){
            auto c = conf.view.sample(x, y, conf.res.width, conf.res.height);
            return escape_time<orbit<std::complex<double>>>({c.convert_to<double>()}, conf.max_iterations);
        },
        [&](std::size_t n){
            auto percent = n / static_cast<float>(conf.max_iterations);
            return (percent == 1.f) ? pixel(0,0,0) : hsv_to_rgb(percent,1.f,1.f);
        }
    );
}

static auto render_perturbed(const render_config& conf, const std::span<pixel>& buffer) -> bool {
    std::vector<std::complex<double>> reference = compute_reference(conf.view.center, conf.max_iterations);
    return render_generic(conf.res, buffer,
        [&](std::size_t x, std::size_t y){
            auto c = conf.view.sample(x, y, conf.res.width, conf.res.height);
            std::complex<double> dc {c - conf.view.center};
            return escape_time<perturbed_orbit>({reference, dc}, conf.max_iterations);
        },
        [&](std::size_t n){
            auto percent = n / static_cast<float>(conf.max_iterations);
            return (percent == 1.f) ? pixel(0,0,0) : hsv_to_rgb(percent,1.f,1.f);
        }
    );
}

static auto render_approximate(const render_config& conf, const std::span<pixel>& buffer) -> bool {
    const auto& sa_conf {std::get<2>(conf.eta)};
    auto reference = compute_reference(conf.view.center, conf.max_iterations);
    series_approximator sa {reference, sa_conf.num_coefficients};
    sa.compute_coeffs_while_valid(
        conf.view.generate_probes(
            sa_conf.probe_cols,
            sa_conf.probe_rows
        ),
        sa_conf.tolerance
    );
    return render_generic(conf.res, buffer,
        [&](std::size_t x, std::size_t y){
            auto c = conf.view.sample(x, y, conf.res.width, conf.res.height);
            std::complex<double> dc {c - conf.view.center};
            auto dz = sa.approximate_delta_n(dc);
            return escape_time<perturbed_orbit>({reference, dz, dc, sa.n()}, conf.max_iterations);
        },
        [&](std::size_t n){
            auto percent = n / static_cast<float>(conf.max_iterations);
            return (percent == 1.f) ? pixel(0,0,0) : hsv_to_rgb(percent,1.f,1.f);
        }
    );
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
auto render(const render_config& conf, const std::span<pixel>& buffer) -> bool {
    return std::visit(overloaded {
        [&](const direct_eta& _)        { (void) _; return render_direct(conf, buffer); },
        [&](const perturbed_eta& _)     { (void) _; return render_perturbed(conf, buffer); },
        [&](const approximate_eta& _)   { (void) _; return render_approximate(conf, buffer); }
    }, conf.eta);
}

}   // namespace wacfrac
