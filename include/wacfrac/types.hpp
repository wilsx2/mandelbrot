#pragma once

#include "wacfrac/floatexp.hpp"
#include <boost/multiprecision/mpc.hpp>
#include <boost/multiprecision/gmp.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/multiprecision/complex_adaptor.hpp>

namespace wacfrac
{

using multi_float   = boost::multiprecision::mpfr_float;
using multi_complex = boost::multiprecision::mpc_complex;
using doubleexp_complex = boost::multiprecision::number<boost::multiprecision::backends::complex_adaptor<doubleexp>>;
// using singleexp_complex = boost::multiprecision::number<boost::multiprecision::backends::complex_adaptor<singleexp>>; Unused
// using quadexp_complex   = boost::multiprecision::complex_adaptor<quadexp>; Unused

namespace detail {

template <typename T, typename = void>
struct complex_value_type_impl {};

template <typename T>
struct complex_value_type_impl<T, std::void_t<typename T::value_type>> {
    using type = typename T::value_type;
};

} // namespace detail

template <typename T>
struct complex_value_type : detail::complex_value_type_impl<T> {};

template <typename T>
using complex_value_type_t = typename complex_value_type<T>::type;

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
auto to_real(const multi_float& val) -> Real {
    using number_doubleexp = boost::multiprecision::number<doubleexp>;
    if constexpr (std::is_same_v<Real, doubleexp>) {
        if (val.is_zero()) return doubleexp{};
        mpfr_exp_t e;
        double m = mpfr_get_d_2exp(&e, val.backend().data(), MPFR_RNDN);
        doubleexp result;
        result.mantissa = m;
        result.exponent = static_cast<int64_t>(e);
        return result;
    } else if constexpr (std::is_same_v<Real, number_doubleexp>) {
        return number_doubleexp{to_real<doubleexp>(val)};
    } else {
        return static_cast<Real>(val);
    }
}

template <typename Real, typename Number>
auto to_real(const Number& val) -> Real {
    using number_doubleexp = boost::multiprecision::number<doubleexp>;
    if constexpr (std::is_same_v<Real, doubleexp> || std::is_same_v<Real, number_doubleexp>) {
        return to_real<Real>(static_cast<multi_float>(val));
    } else {
        return static_cast<Real>(val);
    }
}

template <Complex T>
auto to_complex(multi_complex z) -> T {
    using CT = complex_value_type_t<T>;
    return T{
        to_real<CT>(boost::multiprecision::real(z)),
        to_real<CT>(boost::multiprecision::imag(z))
    };
}

template <>
inline auto to_complex<multi_complex>(multi_complex z) -> multi_complex {
    return z;
}

}   // namespace wacfrac
