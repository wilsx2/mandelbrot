#pragma once

#include "wacfrac/floatexp.hpp"
#include <boost/multiprecision/mpc.hpp>
#include <boost/multiprecision/gmp.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/multiprecision/complex_adaptor.hpp>

namespace wacfrac
{

using MultiFloat   = boost::multiprecision::mpfr_float;
using MultiComplex = boost::multiprecision::mpc_complex;
using DoubleExpComplex = boost::multiprecision::number<boost::multiprecision::backends::complex_adaptor<DoubleExp>>;
// using SingleExpComplex = boost::multiprecision::number<boost::multiprecision::backends::complex_adaptor<SingleExp>>; Unused
// using QuadExpComplex   = boost::multiprecision::complex_adaptor<QuadExp>; Unused

namespace detail {

template <typename T, typename = void>
struct ComplexValueTypeImpl {};

template <typename T>
struct ComplexValueTypeImpl<T, std::void_t<typename T::value_type>> {
    using type = typename T::value_type;
};

} // namespace detail

template <typename T>
struct ComplexValueType : detail::ComplexValueTypeImpl<T> {};

template <typename T>
using ComplexValueTypeT = typename ComplexValueType<T>::type;

template <typename T>
concept Complex = requires(T a, T b) {
    { a.real() };
    { a.imag() };
    T{0.0, 0.0};
    { a + b } -> std::convertible_to<T>;
    { a - b } -> std::convertible_to<T>;
    { a * b } -> std::convertible_to<T>;
    { a / b } -> std::convertible_to<T>;
    { -a } -> std::convertible_to<T>;
    { a += b } -> std::same_as<T&>;
    { a -= b } -> std::same_as<T&>;
    { a *= b } -> std::same_as<T&>;
    { a /= b } -> std::same_as<T&>;
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

template <Complex T>
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
