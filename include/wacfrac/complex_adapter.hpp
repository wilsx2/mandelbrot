#pragma once

#include "wacfrac/hd_macro.hpp"
#include <cmath>
#include <format>

namespace wacfrac {

template<typename T>
class Complex {
    public:
    using value_type = T;

    private:
    T _real, _imag;

    public:
    WACFRAC_HD Complex() : _real(0), _imag(0) {}
    WACFRAC_HD Complex(const T& n) : _real(n), _imag(0) {}
    WACFRAC_HD Complex(const T& r, const T& i) : _real(r), _imag(i) {}
    Complex(const Complex& rhs) = default;
    template<typename U>
    WACFRAC_HD Complex(const Complex<U>& other)
        : _real(static_cast<T>(other.real())), _imag(static_cast<T>(other.imag())) {}
    Complex& operator=(const Complex&) = default;
    WACFRAC_HD
    auto& operator=(const T& n) {
        _real = n;
        _imag = 0;
        return *this;
    }
    WACFRAC_HD
    auto& operator+=(const T& rhs) noexcept {
        _real += rhs;
        return *this;
    }
    WACFRAC_HD
    auto& operator-=(const T& rhs) noexcept {
        _real -= rhs;
        return *this;
    }
    WACFRAC_HD
    auto& operator*=(const T& rhs) noexcept {
        _real *= rhs;
        _imag *= rhs;
        return *this;
    }
    WACFRAC_HD
    auto& operator/=(const T& rhs) {
        _real /= rhs;
        _imag /= rhs;
        return *this;
    }
    WACFRAC_HD
    auto& operator+=(const Complex& rhs) noexcept {
        _real += rhs._real;
        _imag += rhs._imag;
        return *this;
    }
    WACFRAC_HD
    auto& operator-=(const Complex& rhs) noexcept {
        _real -= rhs._real;
        _imag -= rhs._imag;
        return *this;
    }
    WACFRAC_HD
    auto& operator*=(const Complex& rhs) noexcept {
        auto r = _real*rhs._real - _imag*rhs._imag;
        auto i = _real*rhs._imag + _imag*rhs._real;
        _real = r;
        _imag = i;
        return *this;
    }
    WACFRAC_HD
    auto& operator/=(const Complex& rhs) {
        auto denominator {rhs._real*rhs._real + rhs._imag*rhs._imag};
        auto r {(_real*rhs._real + _imag*rhs._imag) / denominator};
        auto i {(_imag*rhs._real - _real*rhs._imag) / denominator};
        _real = r;
        _imag = i;
        return *this;
    }
    WACFRAC_HD
    auto& operator+() {
        return *this;
    }
    WACFRAC_HD
    auto operator-() -> Complex {
        return {-_real, -_imag};
    }
    WACFRAC_HD
    auto& real() {
        return _real;
    }
    WACFRAC_HD
    auto& imag() {
        return _imag;
    }
    WACFRAC_HD
    const auto& real() const {
        return _real;
    }
    WACFRAC_HD
    const auto& imag() const {
        return _imag;
    }

};

template<typename T>
WACFRAC_HD 
inline auto operator+(Complex<T> lhs, const Complex<T>& rhs) {
    return lhs += rhs;
}

template<typename T>
WACFRAC_HD 
inline auto operator-(Complex<T> lhs, const Complex<T>& rhs) {
    return lhs -= rhs;
}

template<typename T>
WACFRAC_HD 
inline auto operator*(Complex<T> lhs, const Complex<T>& rhs) {
    return lhs *= rhs;
}

template<typename T>
WACFRAC_HD 
inline auto operator/(Complex<T> lhs, const Complex<T>& rhs) {
    return lhs /= rhs;
}

template<typename T>
WACFRAC_HD 
inline auto operator+(Complex<T> lhs, const T& rhs) {
    return lhs += rhs;
}

template<typename T>
WACFRAC_HD 
inline auto operator+(const T& lhs, Complex<T> rhs) {
    return rhs += lhs;
}

template<typename T>
WACFRAC_HD 
inline auto operator-(Complex<T> lhs, const T& rhs) {
    return lhs -= rhs;
}

template<typename T>
WACFRAC_HD 
inline auto operator-(const T& lhs, Complex<T> rhs) {
    return -(rhs -= lhs);
}

template<typename T>
WACFRAC_HD 
inline auto operator*(Complex<T> lhs, const T& rhs) {
    return lhs *= rhs;
}

template<typename T>
WACFRAC_HD 
inline auto operator*(const T& lhs, Complex<T> rhs) {
    return rhs *= lhs;
}

template<typename T>
WACFRAC_HD 
inline auto operator/(Complex<T> lhs, const T& rhs) {
    return lhs /= rhs;
}

template<typename T>
WACFRAC_HD 
inline auto abs(const Complex<T>& a) {
    using std::sqrt;
    return sqrt(a.real()*a.real() + a.imag()*a.imag());
}

template<typename T>
WACFRAC_HD 
inline auto norm(const Complex<T>& a) {
    return a.real()*a.real() + a.imag()*a.imag();
}

} // namespace wacfrac

template <typename T>
struct std::formatter<wacfrac::Complex<T>> : std::formatter<std::string> {
    auto format(const wacfrac::Complex<T>& c, std::format_context& ctx) const {
        return std::formatter<std::string>::format(
            std::format("{} {} {}", c.real(), c.imag() >= 0.0 ? "+" : "-", abs(c.imag())),
            ctx);
    }
};
