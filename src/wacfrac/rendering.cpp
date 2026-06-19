#include "wacfrac/rendering.hpp"
#include "wacfrac/color.hpp"
#include "wacfrac/orbit.hpp"
#include "wacfrac/analysis.hpp"
#include "wacfrac/approximation.hpp"
#include "wacfrac/bla.hpp"
#include <algorithm>
#include <complex>
#include <print>

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

// https://philthompson.me/2023/Faster-Mandelbrot-Set-Rendering-with-BLA-Bivariate-Linear-Approximation.html
static auto render_bla(const render_config& conf, const std::span<pixel>& buffer) -> bool {
    auto half_dx = conf.view.dimensions.real() / 2.0;
    auto half_dy = conf.view.dimensions.imag() / 2.0;
    auto periods = find_period_ball(conf.view.center, half_dx, half_dy, conf.max_iterations, true);
    auto view_period = periods.empty() ? 1uz : periods.front();

    auto c_ref {find_nucleus(conf.view.center, view_period, 255uz)};
    auto reference {compute_reference(c_ref, conf.max_iterations)};
    auto c_ref_d {static_cast<std::complex<double>>(c_ref)};

    std::println("ref size: {}, period: {}", reference.size(), view_period);

    for (auto p : periods) {
        if (p == view_period) continue;
        c_ref = find_nucleus(conf.view.center, p, 255uz);
        reference = compute_reference(c_ref, conf.max_iterations);
        c_ref_d = static_cast<std::complex<double>>(c_ref);
        auto nondegenerate = std::ranges::any_of(reference, [](auto z) { return std::abs(z) >= 1e-4; }); // TODO: See below
        if (nondegenerate) {
            view_period = p;
            std::println("trying deeper period: {}", view_period);
            break;
        }
    }

    auto degenerate = std::ranges::none_of(reference, [](auto z) { return std::abs(z) >= 1e-4; }); // TODO: Parameterize degeneration threshhold
    if (reference.empty() || degenerate) {
        std::println("all periods degenerate, falling back to view center");
        c_ref = conf.view.center;
        c_ref_d = static_cast<std::complex<double>>(c_ref);
        reference = compute_reference(c_ref, conf.max_iterations);
    }

    auto c_tr = static_cast<std::complex<double>>(conf.view.sample(conf.res.width, conf.res.height, conf.res.width, conf.res.height));
    auto c_tl = static_cast<std::complex<double>>(conf.view.sample(0, conf.res.height, conf.res.width, conf.res.height));
    auto c_br = static_cast<std::complex<double>>(conf.view.sample(conf.res.width, 0, conf.res.width, conf.res.height));
    auto c_bl = static_cast<std::complex<double>>(conf.view.sample(0, 0, conf.res.width, conf.res.height));

    auto max_dc = c_tr - c_ref_d;
    if (std::abs(c_tl - c_ref_d) > std::abs(max_dc)) max_dc = c_tl - c_ref_d;
    if (std::abs(c_br - c_ref_d) > std::abs(max_dc)) max_dc = c_br - c_ref_d;
    if (std::abs(c_bl - c_ref_d) > std::abs(max_dc)) max_dc = c_bl - c_ref_d;

    const auto& bla_conf {std::get<3>(conf.eta)};
    bivariate_linear_approximator bla {
        bla_conf.epsilon,
        max_dc,
        reference,
        bla_conf.first_level
    };
    return render_generic(conf, buffer,
        [&](std::size_t x, std::size_t y){
            auto c = conf.view.sample(x, y, conf.res.width, conf.res.height);
            std::complex<double> dc {c - c_ref};
            auto [dz, n, _] = bla.escape_approximate(dc);
            return std::make_pair(dz, n);
        }
    );
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
auto render(const render_config& conf, const std::span<pixel>& buffer) -> bool {
    return std::visit(overloaded {
        [&](const direct_eta& _)        { (void) _; return render_direct(conf, buffer); },
        [&](const perturbed_eta& _)     { (void) _; return render_perturbed(conf, buffer); },
        [&](const approximate_eta& _)   { (void) _; return render_approximate(conf, buffer); },
        [&](const bla_eta& _)           { (void) _; return render_bla(conf, buffer); }
    }, conf.eta);
}

}   // namespace wacfrac
