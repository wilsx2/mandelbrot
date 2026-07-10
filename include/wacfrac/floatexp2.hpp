#pragma once

#include "wacfrac/macros.hpp"
#include <cmath>
#include <format>

namespace wacfrac {

template<std::floating_point M, std::integral E>
struct FloatExp {
    public:
    using mantissa_type = M;
    using exponent_type = E;

    private:

    public:
    M val;
    E exp;

    // POD type
    FloatExp() = default;
    FloatExp(const FloatExp& rhs) = default;
    FloatExp(FloatExp&& rhs) = default;
    FloatExp& operator=(const FloatExp&) = default;
    FloatExp& operator=(FloatExp&&) = default;
    ~FloatExp() = default;

	static constexpr E        EXPONENT_MASK   = sizeof(M) == 8 ? 0x7FF0000000000000LL : 0x7F800000;
	static constexpr E        EXPONENT_UNMASK = sizeof(M) == 8 ? 0x800FFFFFFFFFFFFFLL : 0x803FFFFF;
	static constexpr E        EXPONENT_SET    = sizeof(M) == 8 ? 0x3FF0000000000000LL : 0x3F800000;
	static constexpr int      EXPONENT_SHIFT  = sizeof(M) == 8 ?                   52 :         23;
	static constexpr int      EXPONENT_BIAS   = sizeof(M) == 8 ?                 1023 :        127;

	static constexpr E MAX_PREC = sizeof(M) == 8 ? 53 : 24;
	// KF: MIN_EXPONENT is smaller than you might expect, this is to give headroom for
	// avoiding overflow in + and other functions. it is the exponent for 0.0
	static constexpr E EXP_MIN = sizeof(E) == 8 ? E(-0x800000000000000LL) : E(-0x8000000);
	static constexpr E EXP_MAX = -EXP_MIN;

    WF_HD
    inline void align() noexcept {
        E val_i;
        WF_STD::memcpy(&val_i, &val, sizeof(val_i));
        const E e = val_i & EXPONENT_MASK;

        if (e == 0) [[unlikely]] {
            val = 0;
            exp = EXP_MIN;
        } else {
            exp += (e >> EXPONENT_SHIFT) - EXPONENT_BIAS;
            val_i = (val_i & EXPONENT_UNMASK) | EXPONENT_SET;
            WF_STD::memcpy(&val, &val_i, sizeof(val));
        }
    }

    WF_HD
    static inline M set_exp(M val, E exp) noexcept {
        E val_i;
        WF_STD::memcpy(&val_i, &val, sizeof(val_i));
        val_i = (val_i & EXPONENT_UNMASK) | ((exp  + EXPONENT_BIAS) << EXPONENT_SHIFT);
        WF_STD::memcpy(&val, &val_i, sizeof(val));
        return val;
    }

    template<typename T>
    FloatExp& operator=(const T& rhs) { *this = rhs; }
    WF_HD
    auto& operator=(const T& n) {
        _real = n;
        _imag = 0;
        return *this;
    }
    WF_HD
    auto& operator+=(const T& rhs) noexcept {
        _real += rhs;
        return *this;
    }
    WF_HD
    auto& operator-=(const T& rhs) noexcept {
        _real -= rhs;
        return *this;
    }
    WF_HD
    auto& operator*=(const T& rhs) noexcept {
        _real *= rhs;
        _imag *= rhs;
        return *this;
    }
    WF_HD
    auto& operator/=(const T& rhs) {
        _real /= rhs;
        _imag /= rhs;
        return *this;
    }
    WF_HD
    auto& operator+=(const Complex& rhs) noexcept {
        _real += rhs._real;
        _imag += rhs._imag;
        return *this;
    }
    WF_HD
    auto& operator-=(const Complex& rhs) noexcept {
        _real -= rhs._real;
        _imag -= rhs._imag;
        return *this;
    }
    WF_HD
    auto& operator*=(const Complex& rhs) noexcept {
        auto r = _real*rhs._real - _imag*rhs._imag;
        auto i = _real*rhs._imag + _imag*rhs._real;
        _real = r;
        _imag = i;
        return *this;
    }
    WF_HD
    auto& operator/=(const Complex& rhs) {
        auto denominator {rhs._real*rhs._real + rhs._imag*rhs._imag};
        auto r {(_real*rhs._real + _imag*rhs._imag) / denominator};
        auto i {(_imag*rhs._real - _real*rhs._imag) / denominator};
        _real = r;
        _imag = i;
        return *this;
    }
    WF_HD
    auto& operator+() {
        return *this;
    }
    WF_HD
    auto operator-() -> Complex {
        return {-_real, -_imag};
    }
    // TODO: Spaceship
    WF_HD
    auto& real() {
        return _real;
    }
    WF_HD
    auto& imag() {
        return _imag;
    }
    WF_HD
    const auto& real() const {
        return _real;
    }
    WF_HD
    const auto& imag() const {
        return _imag;
    }

};

template<typename T>
WF_HD 
inline auto operator+(Complex<T> lhs, const Complex<T>& rhs) {
    return lhs += rhs;
}

template<typename T>
WF_HD 
inline auto operator-(Complex<T> lhs, const Complex<T>& rhs) {
    return lhs -= rhs;
}

template<typename T>
WF_HD 
inline auto operator*(Complex<T> lhs, const Complex<T>& rhs) {
    return lhs *= rhs;
}

template<typename T>
WF_HD 
inline auto operator/(Complex<T> lhs, const Complex<T>& rhs) {
    return lhs /= rhs;
}

template<typename T>
WF_HD 
inline auto operator+(Complex<T> lhs, const T& rhs) {
    return lhs += rhs;
}

template<typename T>
WF_HD 
inline auto operator+(const T& lhs, Complex<T> rhs) {
    return rhs += lhs;
}

template<typename T>
WF_HD 
inline auto operator-(Complex<T> lhs, const T& rhs) {
    return lhs -= rhs;
}

template<typename T>
WF_HD 
inline auto operator-(const T& lhs, Complex<T> rhs) {
    return -(rhs -= lhs);
}

template<typename T>
WF_HD 
inline auto operator*(Complex<T> lhs, const T& rhs) {
    return lhs *= rhs;
}

template<typename T>
WF_HD 
inline auto operator*(const T& lhs, Complex<T> rhs) {
    return rhs *= lhs;
}

template<typename T>
WF_HD 
inline auto operator/(Complex<T> lhs, const T& rhs) {
    return lhs /= rhs;
}

template<typename T>
WF_HD 
inline auto abs(const Complex<T>& a) {
    using std::sqrt;
    return sqrt(a.real()*a.real() + a.imag()*a.imag());
}

template<typename T>
WF_HD 
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
