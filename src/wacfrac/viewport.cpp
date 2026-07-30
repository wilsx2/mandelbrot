#include "wacfrac/viewport.hpp"

#include "wacfrac/resolution.hpp"
#include "wacfrac/types.hpp"

#include <cmath>

namespace wacfrac
{

    Viewport::Viewport(const MultiComplex& center, const MultiComplex& dimensions)
        : center(center),
          dimensions(dimensions)
    {
    }
    Viewport::Viewport(const MultiComplex& center, const MultiFloat& zoom, const Resolution& res)
        : center(center)
    {
        auto aspect_ratio = res.width / static_cast<double>(res.height);
        if (aspect_ratio >= 1.0)
        {
            dimensions = {aspect_ratio, 1.0};
        }
        else
        {
            dimensions = {1.0, 1.0 / aspect_ratio};
        }
        dimensions /= zoom;
    }

    void Viewport::precision(std::size_t value)
    {
        center.precision(static_cast<unsigned>(value));
        dimensions.precision(static_cast<unsigned>(value));
    }

    auto Viewport::compute_max_dc(MultiComplex c) const -> MultiComplex
    {
        MultiComplex c_tl{center - dimensions / 2.0};
        MultiComplex c_tr{center.real() + dimensions.real() / 2.0, center.imag() - dimensions.imag() / 2.0};
        MultiComplex c_br{center + dimensions / 2.0};
        MultiComplex c_bl{center.real() - dimensions.real() / 2.0, center.imag() + dimensions.imag() / 2.0};

        using boost::multiprecision::abs;
        auto max_dc{MultiComplex{c_tl - c}};
        if (abs(MultiComplex(c_tr - c)) > abs(max_dc))
            max_dc = MultiComplex(c_tr - c);
        if (abs(MultiComplex(c_br - c)) > abs(max_dc))
            max_dc = MultiComplex(c_br - c);
        if (abs(MultiComplex(c_bl - c)) > abs(max_dc))
            max_dc = MultiComplex(c_bl - c);
        return max_dc;
    }

    auto Viewport::required_precision() const -> std::size_t
    {
        return wacfrac::required_precision(dimensions.real());
    }

    auto Viewport::required_iterations(double modifier, double factor, double exponent) const -> unsigned
    {
        return wacfrac::required_iterations(1.0 / dimensions.real(), modifier, factor, exponent);
    }

    auto required_precision(MultiFloat zoom) -> std::size_t
    {
        using namespace std;
        using namespace boost::multiprecision;
        double e{abs(log10(zoom)) + 5.0};
        return static_cast<std::size_t>(max(10.0, e));
    }
    auto required_iterations(MultiFloat zoom, double modifier, double factor, double exponent) -> unsigned
    {
        if (zoom < 1.0)
            return static_cast<unsigned>(modifier);
        return static_cast<unsigned>(
            modifier + factor * std::pow(static_cast<double>(boost::multiprecision::log10(zoom)), exponent));
    }

} // namespace wacfrac
