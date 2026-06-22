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

template <Complex T>
auto to_complex(multi_complex z) -> T {
    using CT = complex_value_type_t<T>;
    return T{
        static_cast<CT>(boost::multiprecision::real(z)),
        static_cast<CT>(boost::multiprecision::imag(z))
    };
}

}   // namespace wacfrac
