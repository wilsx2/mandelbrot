#include <wacfrac/viewport.hpp>
#include <wacfrac/log.hpp>
#include <wacfrac/orbit.hpp>
#include <wacfrac/analysis.hpp>

namespace wacfrac {

template<Complex T>
auto viewport::generate_probes(std::size_t cols, std::size_t rows) const -> std::vector<T> {
    using CT = complex_value_type_t<T>;
    std::vector<T> probes;
    T dimensions_t {
        static_cast<CT>(dimensions.real()),
        static_cast<CT>(dimensions.imag())
    };
    T interval {
        static_cast<CT>(dimensions_t.real()/cols),
        static_cast<CT>(dimensions_t.imag()/rows)
    };
    if (interval.real() == 0.0 || interval.imag() == 0.0) {
        probes.emplace_back(0.0, 0.0);
        return probes;
    }
    T start {
        static_cast<CT>(cols % 2 == 0 ? -dimensions_t.real()/2.0 : (-dimensions_t.real() + interval.real())/2.0),
        static_cast<CT>(rows % 2 == 0 ? -dimensions_t.imag()/2.0 : (-dimensions_t.imag() + interval.imag())/2.0)
    };
    T end {
        static_cast<CT>(+dimensions_t.real() / 2.0),
        static_cast<CT>(+dimensions_t.imag() / 2.0)
    };
    for (CT dx = start.real(); dx <= end.real(); dx += interval.real()) {
        for (CT dy = start.imag(); dy <= end.imag(); dy += interval.imag()) {
            probes.emplace_back(dx, dy);
        }
    }
    return probes;
}

// https://philthompson.me/2023/Faster-Mandelbrot-Set-Rendering-with-BLA-Bivariate-Linear-Approximation.html
template<Complex T>
auto viewport::find_periodic_reference(std::size_t max_n, std::size_t find_period_iter, std::size_t find_nucleus_iter) const -> std::pair<multi_complex, std::vector<T>> {
    auto half_dx = dimensions.real() / 2.0;
    auto half_dy = dimensions.imag() / 2.0;
    LOG_INFO << "Searching for periodic reference (max_n=" << max_n
             << ", period_iter=" << find_period_iter
             << ", nucleus_iter=" << find_nucleus_iter << ")";
    auto periods = find_period_ball(center, half_dx, half_dy, find_period_iter, true);
    auto view_period = periods.empty() ? 1uz : periods.front();
    LOG_INFO << "View period=" << view_period << " (" << periods.size() << " candidates)";

    // Find first non-degenerate reference
    auto c_ref {find_nucleus(center, view_period, find_nucleus_iter)};
    auto reference {compute_reference<T>(c_ref, max_n, false)};

    for (auto p : periods) {
        if (p == view_period) continue;
        LOG_DEBUG << "Trying period " << p << " for non-degenerate reference";
        c_ref = find_nucleus(center, p, find_nucleus_iter);
        reference = compute_reference<T>(c_ref, max_n, false);
        if (!is_reference_degenerate(reference)) {
            LOG_DEBUG << "Found non-degenerate reference at period " << p;
            view_period = p;
            break;
        }
    }

    // Fallback to center if none found
    auto degenerate = is_reference_degenerate(reference); 
    if (reference.empty() || degenerate) {
        LOG_WARN << "No suitable periodic reference found, falling back to view center";
        c_ref = center;
        reference = compute_reference<T>(c_ref, max_n, false);
    }

    return std::make_pair(c_ref, reference);
}

} // namespace wacfrac 
