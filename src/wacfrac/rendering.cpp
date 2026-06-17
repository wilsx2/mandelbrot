#include "wacfrac/color.hpp"
#include <wacfrac/rendering.hpp>
#include <wacfrac/approximation.hpp>

namespace wacfrac
{

template <std::invocable<std::size_t, std::size_t> F>
static auto render_generic(const render_config& conf, const std::span<pixel>& buffer, F&& escape_fn) -> bool {
    if (conf.res.area() != buffer.size())
        return false;
    auto i {0uz};
    for (auto [y, x] : conf.res.coordinates()) {
        std::complex<double> z;
        unsigned int n;
        std::tie(z,n) = escape_fn(x, y);
        buffer[i++] = colorize(conf.ca, conf.palette, conf.max_iterations, z, n);
    }
    return true;
}

static auto render_direct(const render_config& conf, const std::span<pixel>& buffer) -> bool {
    return render_generic(conf, buffer,
        [&](std::size_t x, std::size_t y){
            auto c = conf.view.sample(x, y, conf.res.width, conf.res.height);
            return escape<multi_complex>(c, conf.max_iterations);
        }
    );
}

static auto render_perturbed(const render_config& conf, const std::span<pixel>& buffer) -> bool {
    std::vector<std::complex<double>> reference = compute_reference(conf.view.center, conf.max_iterations);
    return render_generic(conf, buffer,
        [&](std::size_t x, std::size_t y){
            auto c = conf.view.sample(x, y, conf.res.width, conf.res.height);
            std::complex<double> dc {c - conf.view.center};
            return escape_perturbed(reference, dc, conf.max_iterations);
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
    return render_generic(conf, buffer,
        [&](std::size_t x, std::size_t y){
            auto c = conf.view.sample(x, y, conf.res.width, conf.res.height);
            std::complex<double> dc {c - conf.view.center};
            auto dz = sa.approximate_delta_n(dc);
            return escape_perturbed(reference, dc, conf.max_iterations, dz, sa.n());
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
