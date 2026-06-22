#include <wacfrac/viewport.hpp>

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

} // namespace wacfrac 
