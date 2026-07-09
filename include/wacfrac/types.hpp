#pragma once

#include "wacfrac/complex_adapter.hpp"
#include "wacfrac/complex_concept.hpp"
#include "wacfrac/floatexp.hpp"

#include <boost/multiprecision/fwd.hpp>
#include <boost/multiprecision/mpc.hpp>
#include <boost/multiprecision/gmp.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <boost/multiprecision/complex_adaptor.hpp>

namespace wacfrac
{

// Numeric Types
using SingleExp = FloatExp<float,  int64_t>;
using DoubleExp = FloatExp<double, int64_t>;
using MultiFloat   = boost::multiprecision::mpfr_float;

// Complex Types
using SingleComplex = Complex<float>;
using DoubleComplex = Complex<double>;
using MultiComplex  = boost::multiprecision::mpc_complex;
using SingleExpComplex = boost::multiprecision::number<boost::multiprecision::backends::complex_adaptor<SingleExp>>;
using DoubleExpComplex = boost::multiprecision::number<boost::multiprecision::backends::complex_adaptor<DoubleExp>>;

struct ReferenceSet {
    std::vector<SingleComplex> float_ref;
    std::vector<DoubleComplex> double_ref;
    std::vector<DoubleExpComplex> dexp_ref;

    template <typename T>
    auto select() const -> const std::vector<T>& {
        if constexpr (std::is_same_v<T, SingleComplex>)
            return float_ref;
        else if constexpr (std::is_same_v<T, DoubleComplex>)
            return double_ref;
        else
            return dexp_ref;
    }

    template <typename T>
    auto select() -> std::vector<T>& {
        if constexpr (std::is_same_v<T, SingleComplex>)
            return float_ref;
        else if constexpr (std::is_same_v<T, DoubleComplex>)
            return double_ref;
        else
            return dexp_ref;
    }

    void reserve(std::size_t size) {
        select<SingleComplex>().reserve(size);
        select<DoubleComplex>().reserve(size);
        select<DoubleExpComplex>().reserve(size);
    }
};

template <typename Real>
auto to_real(const MultiFloat& val) -> Real {
    using NumberDoubleExp = boost::multiprecision::number<DoubleExp>;
    if constexpr (std::is_same_v<Real, DoubleExp>) {
        if (val.is_zero()) return DoubleExp{};
        mpfr_exp_t e;
        double m = mpfr_get_d_2exp(&e, val.backend().data(), MPFR_RNDN);
        DoubleExp result;
        result.mantissa = m;
        result.exponent = static_cast<int64_t>(e);
        return result;
    } else if constexpr (std::is_same_v<Real, NumberDoubleExp>) {
        return NumberDoubleExp{to_real<DoubleExp>(val)};
    } else {
        return static_cast<Real>(val);
    }
}

template <typename Real, typename Number>
auto to_real(const Number& val) -> Real {
    using NumberDoubleExp = boost::multiprecision::number<DoubleExp>;
    if constexpr (std::is_same_v<Real, DoubleExp> || std::is_same_v<Real, NumberDoubleExp>) {
        return to_real<Real>(static_cast<MultiFloat>(val));
    } else {
        return static_cast<Real>(val);
    }
}

template <ComplexConcept T>
auto to_complex(MultiComplex z) -> T {
    using CT = ComplexValueTypeT<T>;
    return T{
        to_real<CT>(boost::multiprecision::real(z)),
        to_real<CT>(boost::multiprecision::imag(z))
    };
}

template <>
inline auto to_complex<MultiComplex>(MultiComplex z) -> MultiComplex {
    return z;
}

}   // namespace wacfrac
