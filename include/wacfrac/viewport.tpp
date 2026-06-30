#include <boost/multiprecision/detail/default_ops.hpp>
#include <ranges>
#include <wacfrac/viewport.hpp>
#include <wacfrac/log.hpp>
#include <wacfrac/orbit.hpp>
#include <wacfrac/analysis.hpp>
#include <limits>

namespace wacfrac {

template<Complex T>
auto Viewport::generate_probes(std::size_t cols, std::size_t rows) const -> std::vector<T> {
    using CT = ComplexValueTypeT<T>;
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
auto Viewport::find_periodic_reference(std::size_t max_n, std::size_t find_nucleus_iter) const -> std::pair<MultiComplex, std::vector<T>> {
    using boost::multiprecision::isnan;
    logging::info( "Searching for periodic reference (max_n={}, nucleus_iter={})", max_n, find_nucleus_iter);

    auto half_dx = dimensions.real() / 2.0;
    auto half_dy = dimensions.imag() / 2.0;

    auto inf {std::numeric_limits<double>::infinity()};
    // Find first non-degenerate reference
    std::size_t period;
    PeriodFinder iter {center, half_dx, half_dy, max_n};
    while ((period = iter.next()) != 0) {
        logging::debug( "Trying period {} for non-degenerate reference", period);
        auto c_ref {find_nucleus(center, period, find_nucleus_iter)};
        if (isnan(c_ref.real()) || isnan(c_ref.imag())) {
            logging::debug( "Failed to find nucleus");
            continue;
        }
        logging::debug( "Nucleus found, starting on reference");
        auto reference {compute_reference_mt<T>(c_ref, period, inf)};
        if (!is_reference_degenerate(reference)) {
            logging::debug( "Found non-degenerate reference at period {}", period);
            return std::make_pair(c_ref, reference);
        }
    }

    // Fallback to center if none found
    logging::warning( "No suitable periodic reference found, falling back to view center");
    return std::make_pair(
        center,
        compute_reference_mt<T>(center, max_n, inf)
    );
}

} // namespace wacfrac 
