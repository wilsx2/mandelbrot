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

// Complex Types
using SingleComplex = Complex<float>;
using DoubleComplex = Complex<double>;
using SingleExpComplex = Complex<SingleExp>;
using DoubleExpComplex = Complex<DoubleExp>;

using MultiFloat   = boost::multiprecision::mpfr_float;
using MultiComplex  = boost::multiprecision::mpc_complex;

template <typename Real>
auto to_real(const MultiFloat& val) -> Real {
    using NumberDoubleExp = boost::multiprecision::number<DoubleExp>;
    if constexpr (std::is_same_v<Real, DoubleExp>) {
        if (val.is_zero()) return DoubleExp(0);
        mpfr_exp_t e;
        double m = mpfr_get_d_2exp(&e, val.backend().data(), MPFR_RNDN);
        return DoubleExp(m, static_cast<int64_t>(e));
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
