#pragma once

#include "wacfrac/hd_macro.hpp"
#include <complex>
#include <format>

namespace wacfrac {

template<typename T>
class ComplexAdapter {
    public:
    using value_type = T;

    private:
    T _real, _imag;

    public:
    WACFRAC_HD ComplexAdapter() : _real(0), _imag(0) {}
    WACFRAC_HD ComplexAdapter(const T& n) : _real(n), _imag(0) {}
    WACFRAC_HD ComplexAdapter(const T& r, const T& i) : _real(r), _imag(i) {}
    WACFRAC_HD ComplexAdapter(const ComplexAdapter&) = default;
    WACFRAC_HD ComplexAdapter& operator=(const ComplexAdapter&) = default;
    WACFRAC_HD
    auto& operator=(const T& n) {
        _real = n;
        _imag = 0;
        return *this;
    }
    WACFRAC_HD
    auto& operator+=(const ComplexAdapter& rhs) noexcept {
        _real += rhs._real;
        _imag += rhs._imag;
        return *this;
    }
    WACFRAC_HD
    auto& operator-=(const ComplexAdapter& rhs) noexcept {
        _real -= rhs._real;
        _imag -= rhs._imag;
        return *this;
    }
    WACFRAC_HD
    auto& operator*=(const ComplexAdapter& rhs) noexcept {
        auto r = _real*rhs._real - _imag*rhs._imag;
        auto i = _real*rhs._imag + _imag*rhs._real;
        _real = r;
        _imag = i;
        return *this;
    }
    WACFRAC_HD
    auto& operator/=(const ComplexAdapter& rhs) {
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
    auto operator-() -> ComplexAdapter {
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
    template<typename U> // TODO: Delete
    operator std::complex<U>() const {
        return {static_cast<U>(_real), static_cast<U>(_imag)};
    }
};

template<typename T>
WACFRAC_HD 
inline auto operator+(ComplexAdapter<T> lhs, const ComplexAdapter<T>& rhs) {
    return lhs += rhs;
}

template<typename T>
WACFRAC_HD 
inline auto operator-(ComplexAdapter<T> lhs, const ComplexAdapter<T>& rhs) {
    return lhs -= rhs;
}

template<typename T>
WACFRAC_HD 
inline auto operator*(ComplexAdapter<T> lhs, const ComplexAdapter<T>& rhs) {
    return lhs *= rhs;
}

template<typename T>
WACFRAC_HD 
inline auto operator/(ComplexAdapter<T> lhs, const ComplexAdapter<T>& rhs) {
    return lhs /= rhs;
}

template<typename T>
WACFRAC_HD 
inline auto abs(const ComplexAdapter<T>& a) {
    return a.real()*a.real() + a.imag()*a.imag();
}

template<typename T>
WACFRAC_HD 
inline auto norm(const ComplexAdapter<T>& a) {
    using namespace std;
    return sqrt(abs(a));
}

} // namespace wacfrac

template <typename T>
struct std::formatter<wacfrac::ComplexAdapter<T>> : std::formatter<std::string> {
    auto format(const wacfrac::ComplexAdapter<T>& c, std::format_context& ctx) const {
        return std::formatter<std::string>::format(
            std::format("{} {} {}", c.real(), c.imag() >= 0.0 ? "+" : "-", abs(c.imag())),
            ctx);
    }
};
