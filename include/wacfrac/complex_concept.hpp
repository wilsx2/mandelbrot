#pragma once

#include <concepts>
#include <type_traits>

namespace wacfrac {

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
concept ComplexConcept = requires(T a, T b) {
    { a.real() };
    { a.imag() };
    { norm(a) };
    { abs(a) };
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

} // namespace wacfrac
