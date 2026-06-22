#include "wacfrac/rendering.hpp"
#include "wacfrac/color.hpp"
#include "wacfrac/orbit.hpp"
#include "wacfrac/analysis.hpp"
#include "wacfrac/sa.hpp"
#include "wacfrac/bla.hpp"
#include "wacfrac/types.hpp"
#include <algorithm>
#include <boost/multiprecision/fwd.hpp>
#include <boost/multiprecision/number.hpp>
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
        auto [z, n] = escape_fn(x, y);
        buffer[i++] = colorize(conf.ca, conf.palette, conf.max_iterations, z, n);
    }
    return true;
}

template <Complex T>
static auto render_direct(const render_config& conf, const std::span<pixel>& buffer) -> bool {
    return render_generic(conf, buffer,
        [&](std::size_t x, std::size_t y){
            auto raw = conf.view.sample(x, y, conf.res.width, conf.res.height);
            auto c = T{static_cast<std::complex<long double>>(raw)};
            auto [z, n] = escape<T>(c, conf.max_iterations);
            return std::pair<std::complex<long double>, std::size_t>{static_cast<std::complex<long double>>(z), n};
        }
    );
}

template <Complex T>
static auto render_perturbed(const render_config& conf, const std::span<pixel>& buffer) -> bool {
    std::vector<T> reference = compute_reference<T>(conf.view.center, conf.max_iterations);
    return render_generic(conf, buffer,
        [&](std::size_t x, std::size_t y){
            auto c = conf.view.sample(x, y, conf.res.width, conf.res.height);
            T dc {static_cast<T>(c - conf.view.center)};
            auto [z, n] = escape_perturbed<T>(reference, dc, conf.max_iterations);
            return std::pair<std::complex<long double>, std::size_t>{static_cast<std::complex<long double>>(z), n};
        }
    );
}

template <Complex T>
static auto render_sa(const render_config& conf, const std::span<pixel>& buffer) -> bool {
    const auto& sa_conf {std::get<2>(conf.eta)};
    auto reference = compute_reference<T>(conf.view.center, conf.max_iterations);
    series_approximator<T> sa {reference, sa_conf.num_coefficients};
    sa.compute_coeffs_while_valid(
        conf.view.generate_probes<T>(sa_conf.probe_cols, sa_conf.probe_rows),
        sa_conf.tolerance
    );
    return render_generic(conf, buffer,
        [&](std::size_t x, std::size_t y){
            auto c = conf.view.sample(x, y, conf.res.width, conf.res.height);
            T dc {static_cast<T>(c - conf.view.center)};
            auto dz = sa.approximate_delta_n(dc);
            auto [z, n] = escape_perturbed<T>(reference, dc, conf.max_iterations, dz, sa.n());
            return std::pair<std::complex<long double>, std::size_t>{static_cast<std::complex<long double>>(z), n};
        }
    );
}

// https://philthompson.me/2023/Faster-Mandelbrot-Set-Rendering-with-BLA-Bivariate-Linear-Approximation.html
template <Complex T>
static auto render_bla(const render_config& conf, const std::span<pixel>& buffer) -> bool {
    std::println("max_iter: {}", conf.max_iterations);
    std::println("finding period...");
    auto half_dx = conf.view.dimensions.real() / 2.0;
    auto half_dy = conf.view.dimensions.imag() / 2.0;
    auto periods = find_period_ball(conf.view.center, half_dx, half_dy, conf.max_iterations, true);
    auto view_period = periods.empty() ? 1uz : periods.front();

    std::println("finding nucleus...");
    auto c_ref {find_nucleus(conf.view.center, view_period, 256uz)}; // TODO: Parameterize magic numbers
    std::println("calculating reference...");
    auto reference {compute_reference<T>(c_ref, conf.max_iterations, false)};

    std::println("ref size: {}, period: {}", reference.size(), view_period);

    for (auto p : periods) {
        if (p == view_period) continue;
        c_ref = find_nucleus(conf.view.center, p, 256uz); // TODO: Parameterize magic numbers
        reference = compute_reference<T>(c_ref, conf.max_iterations, false);
        auto nondegenerate = std::ranges::any_of(reference, [](auto z) { using std::abs; return abs(z) >= 1e-4; }); // TODO: See below
        if (nondegenerate) {
            view_period = p;
            std::println("trying deeper period: {}", view_period);
            break;
        }
    }

    auto degenerate = std::ranges::none_of(reference, [](auto z) { using std::abs; return abs(z) >= 1e-4; }); // TODO: Parameterize degeneration threshhold
    if (reference.empty() || degenerate) {
        std::println("all periods degenerate, falling back to view center");
        c_ref = conf.view.center;
        reference = compute_reference<T>(c_ref, conf.max_iterations, false);
        std::println("ref size: {}", reference.size(), view_period);
    }

    auto c_tr = (conf.view.sample(conf.res.width, conf.res.height, conf.res.width, conf.res.height));
    auto c_tl = (conf.view.sample(0, conf.res.height, conf.res.width, conf.res.height));
    auto c_br = (conf.view.sample(conf.res.width, 0, conf.res.width, conf.res.height));
    auto c_bl = (conf.view.sample(0, 0, conf.res.width, conf.res.height));

    using boost::multiprecision::abs;
    auto max_dc = multi_complex{c_tr - c_ref};
    if (abs(multi_complex(c_tl - c_ref)) > abs(max_dc)) max_dc = multi_complex(c_tl - c_ref);
    if (abs(multi_complex(c_br - c_ref)) > abs(max_dc)) max_dc = multi_complex(c_br - c_ref);
    if (abs(multi_complex(c_bl - c_ref)) > abs(max_dc)) max_dc = multi_complex(c_bl - c_ref);

    const auto& bla_conf {std::get<3>(conf.eta)};
    /*bivariate_linear_approximator<T> bla { // TODO: Options to select between manual and automatic epsilon
        bla_conf.epsilon,
        static_cast<T>(max_dc),
        reference,
        bla_conf.first_level
    };*/

    std::println("calculating coeffs...");
    bivariate_linear_approximator<T> bla {
        1e-100, 1e-5, // TODO: Replace magic nums
        conf.view.generate_probes<T>(4, 4), // TODO: Replace magic nums
        to_complex<T>(max_dc),
        reference,
        bla_conf.first_level
    };

    std::println("rendering...");
    return render_generic(conf, buffer,
        [&](std::size_t x, std::size_t y){
            auto c = conf.view.sample(x, y, conf.res.width, conf.res.height);
            T dc {to_complex<T>(static_cast<multi_complex>(c - c_ref))};
            std::print("{} + i{},", static_cast<long double>(dc.real()), static_cast<long double>(dc.imag()));
            auto [dz, n, _] = bla.escape_approximate(dc);
            return std::pair<std::complex<long double>, std::size_t>{static_cast<std::complex<long double>>(dz), n};
        }
    );
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
auto render(const render_config& conf, const std::span<pixel>& buffer) -> bool {
    return std::visit(overloaded {
        [&](const direct_eta& _)        { (void) _; return render_direct<doubleexp_complex>(conf, buffer); },
        [&](const perturbed_eta& _)     { (void) _; return render_perturbed<std::complex<long double>>(conf, buffer); },
        [&](const sa_eta& _)            { (void) _; return render_sa<std::complex<long double>>(conf, buffer); },
        [&](const bla_eta& _)           { (void) _; return render_bla<doubleexp_complex>(conf, buffer); }
    }, conf.eta);
}

template static auto render_direct<doubleexp_complex>(const render_config&, const std::span<pixel>&) -> bool;
template static auto render_perturbed<std::complex<long double>>(const render_config&, const std::span<pixel>&) -> bool;
template static auto render_sa<std::complex<long double>>(const render_config&, const std::span<pixel>&) -> bool;
template static auto render_bla<std::complex<long double>>(const render_config&, const std::span<pixel>&) -> bool;

}   // namespace wacfrac
