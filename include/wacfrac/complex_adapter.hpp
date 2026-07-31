#pragma once

#include <cmath>
#include <concepts>
#include <format>
#include <type_traits>

namespace wacfrac
{

template <typename T> class Complex
{
public:
    using value_type = T;

private:
    T _real, _imag;

public:
    SYCL_EXTERNAL Complex()
        : _real(0),
          _imag(0)
    {
    }
    SYCL_EXTERNAL Complex(const T& n)
        : _real(n),
          _imag(0)
    {
    }
    template <typename U>
        requires(std::is_arithmetic_v<U> && !std::same_as<std::remove_cvref_t<U>, T>)
    SYCL_EXTERNAL Complex(U n)
        : _real(static_cast<T>(n)),
          _imag(0)
    {
    }
    SYCL_EXTERNAL Complex(const T& r, const T& i)
        : _real(r),
          _imag(i)
    {
    }
    Complex(const Complex& rhs) = default;
    template <typename U>
    SYCL_EXTERNAL Complex(const Complex<U>& other)
        : _real(static_cast<T>(other.real())),
          _imag(static_cast<T>(other.imag()))
    {
    }
    Complex& operator=(const Complex&) = default;
    SYCL_EXTERNAL
    auto& operator=(const T& n)
    {
        _real = n;
        _imag = 0;
        return *this;
    }
    SYCL_EXTERNAL
    auto& operator+=(const T& rhs) noexcept
    {
        _real += rhs;
        return *this;
    }
    SYCL_EXTERNAL
    auto& operator-=(const T& rhs) noexcept
    {
        _real -= rhs;
        return *this;
    }
    SYCL_EXTERNAL
    auto& operator*=(const T& rhs) noexcept
    {
        _real *= rhs;
        _imag *= rhs;
        return *this;
    }
    SYCL_EXTERNAL
    auto& operator/=(const T& rhs)
    {
        _real /= rhs;
        _imag /= rhs;
        return *this;
    }
    SYCL_EXTERNAL
    auto& operator+=(const Complex& rhs) noexcept
    {
        _real += rhs._real;
        _imag += rhs._imag;
        return *this;
    }
    SYCL_EXTERNAL
    auto& operator-=(const Complex& rhs) noexcept
    {
        _real -= rhs._real;
        _imag -= rhs._imag;
        return *this;
    }
    SYCL_EXTERNAL
    auto& operator*=(const Complex& rhs) noexcept
    {
        auto r = _real * rhs._real - _imag * rhs._imag;
        auto i = _real * rhs._imag + _imag * rhs._real;
        _real = r;
        _imag = i;
        return *this;
    }
    SYCL_EXTERNAL
    auto& operator/=(const Complex& rhs)
    {
        auto denominator{rhs._real * rhs._real + rhs._imag * rhs._imag};
        auto r{(_real * rhs._real + _imag * rhs._imag) / denominator};
        auto i{(_imag * rhs._real - _real * rhs._imag) / denominator};
        _real = r;
        _imag = i;
        return *this;
    }
    SYCL_EXTERNAL
    auto& operator+() { return *this; }
    SYCL_EXTERNAL
    auto operator-() -> Complex { return {-_real, -_imag}; }
    SYCL_EXTERNAL
    auto& real() { return _real; }
    SYCL_EXTERNAL
    auto& imag() { return _imag; }
    SYCL_EXTERNAL
    const auto& real() const { return _real; }
    SYCL_EXTERNAL
    const auto& imag() const { return _imag; }
};

template <typename T> SYCL_EXTERNAL inline auto operator+(Complex<T> lhs, const Complex<T>& rhs)
{
    return lhs += rhs;
}

template <typename T> SYCL_EXTERNAL inline auto operator-(Complex<T> lhs, const Complex<T>& rhs)
{
    return lhs -= rhs;
}

template <typename T> SYCL_EXTERNAL inline auto operator*(Complex<T> lhs, const Complex<T>& rhs)
{
    return lhs *= rhs;
}

template <typename T> SYCL_EXTERNAL inline auto operator/(Complex<T> lhs, const Complex<T>& rhs)
{
    return lhs /= rhs;
}

template <typename T> SYCL_EXTERNAL inline auto operator+(Complex<T> lhs, const T& rhs)
{
    return lhs += rhs;
}

template <typename T> SYCL_EXTERNAL inline auto operator+(const T& lhs, Complex<T> rhs)
{
    return rhs += lhs;
}

template <typename T> SYCL_EXTERNAL inline auto operator-(Complex<T> lhs, const T& rhs)
{
    return lhs -= rhs;
}

template <typename T> SYCL_EXTERNAL inline auto operator-(const T& lhs, Complex<T> rhs)
{
    return -(rhs -= lhs);
}

template <typename T> SYCL_EXTERNAL inline auto operator*(Complex<T> lhs, const T& rhs)
{
    return lhs *= rhs;
}

template <typename T> SYCL_EXTERNAL inline auto operator*(const T& lhs, Complex<T> rhs)
{
    return rhs *= lhs;
}

template <typename T> SYCL_EXTERNAL inline auto operator/(Complex<T> lhs, const T& rhs)
{
    return lhs /= rhs;
}

template <typename T> SYCL_EXTERNAL inline auto abs(const Complex<T>& a)
{
    using std::sqrt;
    return sqrt(a.real() * a.real() + a.imag() * a.imag());
}

template <typename T> SYCL_EXTERNAL inline auto norm(const Complex<T>& a)
{
    return a.real() * a.real() + a.imag() * a.imag();
}

} // namespace wacfrac

template <typename T> struct std::formatter<wacfrac::Complex<T>> : std::formatter<std::string> {
    auto format(const wacfrac::Complex<T>& c, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(
            std::format("{} {} {}", c.real(), c.imag() >= 0.0 ? "+" : "-", abs(c.imag())), ctx);
    }
};
