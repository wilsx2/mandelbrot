#pragma once

#include "wacfrac/macros.hpp"
#include <cmath>
#include <concepts>
#include <climits>
#include <cstdint>
#include <limits>
#if defined(__CUDACC__)
    #include <cuda/std/cstring>
#else
    #include <cstring>
    #include <iomanip>
    #include <sstream>
    #if defined(__cpp_impl_three_way_comparison) && __has_include(<compare>)
        #include <compare>
    #endif
    #if defined(__cpp_lib_format)
        #include <format>
    #endif
#endif

namespace wacfrac {

// Adapted from: https://mathr.co.uk/kf/kf.html

template<std::floating_point M, std::integral E>
struct FloatExp {
    using mantissa_type = M;
    using exponent_type = E;

    M val;
    E exp;

    FloatExp() = default;
    FloatExp(const FloatExp&) = default;
    FloatExp(FloatExp&&) = default;
    FloatExp& operator=(const FloatExp&) = default;
    FloatExp& operator=(FloatExp&&) = default;
    ~FloatExp() = default;

    static constexpr E   EXPONENT_MASK   = sizeof(M) == 8 ? 0x7FF0000000000000LL : 0x7F800000;
    static constexpr E   EXPONENT_UNMASK = sizeof(M) == 8 ? 0x800FFFFFFFFFFFFFLL : 0x803FFFFF;
    static constexpr E   EXPONENT_SET    = sizeof(M) == 8 ? 0x3FF0000000000000LL : 0x3F800000;
    static constexpr int EXPONENT_SHIFT  = sizeof(M) == 8 ?                   52 :         23;
    static constexpr int EXPONENT_BIAS   = sizeof(M) == 8 ?                 1023 :        127;

    static constexpr E MAX_PREC = sizeof(M) == 8 ? 53 : 24;
    // MIN_EXPONENT is smaller than you might expect, this is to give headroom for
    // avoiding overflow in + and other functions. it is the exponent for 0.0
    static constexpr E EXP_MIN = sizeof(E) == 8 ? E(-0x800000000000000LL) : E(-0x8000000);
    static constexpr E EXP_MAX = -EXP_MIN;

    private:
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

    public:
    WF_HD
    static inline M set_exp(M val, E exp) noexcept {
        E val_i;
        WF_STD::memcpy(&val_i, &val, sizeof(val_i));
        val_i = (val_i & EXPONENT_UNMASK) | ((exp  + EXPONENT_BIAS) << EXPONENT_SHIFT);
        WF_STD::memcpy(&val, &val_i, sizeof(val));
        return val;
    }

    template<std::integral I>
    WF_HD
    FloatExp(I a) noexcept : val(a), exp(0) {
        align();
    }

    template<std::floating_point F>
    WF_HD
    FloatExp(F a) noexcept {
        using WF_STD::frexp;
        auto e {0};
        a = frexp(a, &e);
        val = a;
        exp = e;
        align();
    }

    WF_HD
    FloatExp(M a, E e) noexcept : val(a), exp(e) {
        align();
    }

    WF_HD
    FloatExp(M a, E e, int) noexcept : val(a), exp(e) {}

    template<std::floating_point M2, std::integral E2>
    WF_HD
    FloatExp(const FloatExp<M2, E2>& a) noexcept {
        E clamped = static_cast<E>(a.exp);
        clamped = WF_STD::max(clamped, E(EXP_MIN));
        clamped = WF_STD::min(clamped, E(EXP_MAX));
        val = static_cast<M>(a.val);
        exp = clamped;
        align();
    }

    WF_HD
    inline FloatExp& operator *=(const FloatExp& a) noexcept {
        val *= a.val;
        exp += a.exp;
        align();
        return *this;
    }

    WF_HD
    inline FloatExp& operator /=(const FloatExp& a) noexcept {
        val /= a.val;
        exp -= a.exp;
        align();
        return *this;
    }

    WF_HD
    inline FloatExp& operator +=(const FloatExp& a) noexcept {
        *this = *this + a;
        return *this;
    }

    WF_HD
    inline FloatExp& operator -=(const FloatExp& a) noexcept {
        *this = *this - a;
        return *this;
    }

    WF_HD
    inline FloatExp operator *(const FloatExp& a) const noexcept {
        return FloatExp(*this) *= a;
    }

    WF_HD
    inline FloatExp operator /(const FloatExp& a) const noexcept {
        return FloatExp(*this) /= a;
    }

    WF_HD
    inline FloatExp operator +(const FloatExp& a) const noexcept {
        FloatExp r;
        E diff;
        if (exp > a.exp) {
            diff = exp - a.exp;
            r.exp = exp;
            if (diff > MAX_PREC)
                r.val = val;
            else {
                M aval = set_exp(a.val, -diff);
                r.val = val + aval;
            }
        } else {
            diff = a.exp - exp;
            r.exp = a.exp;
            if (diff > MAX_PREC)
                r.val = a.val;
            else {
                M aval = set_exp(val, -diff);
                r.val = a.val + aval;
            }
        }
        r.align();
        return r;
    }

    WF_HD
    inline FloatExp operator -(const FloatExp& a) const noexcept {
        FloatExp r;
        E diff;
        if (exp > a.exp) {
            diff = exp - a.exp;
            r.exp = exp;
            if (diff > MAX_PREC)
                r.val = val;
            else {
                M aval = set_exp(a.val, -diff);
                r.val = val - aval;
            }
        } else {
            diff = a.exp - exp;
            r.exp = a.exp;
            if (diff > MAX_PREC)
                r.val = -a.val;
            else {
                M aval = set_exp(val, -diff);
                r.val = aval - a.val;
            }
        }
        r.align();
        return r;
    }

    WF_HD
    inline FloatExp operator -() const noexcept {
        FloatExp r = *this;
        r.val = -r.val;
        return r;
    }

    WF_HD
    inline FloatExp operator +() const noexcept {
        return *this;
    }

    __attribute__((warn_unused_result))
    WF_HD
    inline FloatExp mul2() const noexcept {
        FloatExp r;
        r.val = val;
        r.exp = exp + 1;
        return r;
    }

    private:
    WF_HD
    inline int compare(const FloatExp& a) const noexcept {
        if (val > 0) {
            if (a.val < 0) return 1;
            if (exp > a.exp) return 1;
            else if (exp < a.exp) return -1;
            return (val > a.val) - (val < a.val);
        } else {
            if (a.val > 0) return -1;
            if (exp > a.exp) return -1;
            else if (exp < a.exp) return 1;
            return (val > a.val) - (val < a.val);
        }
    }

    public:
    WF_HD
    inline bool operator ==(const FloatExp& a) const noexcept {
        if (val != 0 && !WF_STD::isinf(val) && exp != a.exp)
            return false;
        return val == a.val;
    }

    WF_HD
    inline bool operator !=(const FloatExp& a) const noexcept {
        return !(*this == a);
    }

    WF_HD
    inline bool operator >(const FloatExp& a) const noexcept { return compare(a) > 0; }

    WF_HD
    inline bool operator <(const FloatExp& a) const noexcept { return compare(a) < 0; }

    WF_HD
    inline bool operator >=(const FloatExp& a) const noexcept { return compare(a) >= 0; }

    WF_HD
    inline bool operator <=(const FloatExp& a) const noexcept { return compare(a) <= 0; }

#if defined(__cpp_impl_three_way_comparison) && __has_include(<compare>)
    inline std::partial_ordering operator <=>(const FloatExp& a) const noexcept {
        if (WF_STD::isnan(val) || WF_STD::isnan(a.val))
            return std::partial_ordering::unordered;
        int cmp = compare(a);
        if (cmp > 0) return std::partial_ordering::greater;
        if (cmp < 0) return std::partial_ordering::less;
        return std::partial_ordering::equivalent;
    }
#endif

    WF_HD
    inline operator float() const noexcept {
        using WF_STD::ldexp;
        if (exp > E(INT_MAX))
            return float(val) / 0.0f;
        if (exp < E(INT_MIN))
            return float(val) * 0.0f;
        return ldexp(float(val), int(exp));
    }

    WF_HD
    inline operator double() const noexcept {
        using WF_STD::ldexp;
        if (exp > E(INT_MAX))
            return double(val) / 0.0;
        if (exp < E(INT_MIN))
            return double(val) * 0.0;
        return ldexp(double(val), int(exp));
    }

#if !defined(__CUDACC__)
    inline std::string toString(int digits = 0) const noexcept {
        using WF_STD::log10;
        using WF_STD::abs;
        using WF_STD::floor;
        using WF_STD::pow;
        if (WF_STD::isnan(val)) return "nan";
        if (WF_STD::isinf(val)) return val > 0 ? "+inf" : "-inf";
        M lf = log10(abs(val)) + M(exp) * log10(M(2));
        E e10 = E(floor(lf));
        M d10 = pow(M(10), lf - e10) * M((val > 0) - (val < 0));
        if (val == 0) { d10 = 0; e10 = 0; }
        std::ostringstream os;
        os << std::setprecision(digits ? digits : (std::numeric_limits<M>::digits10 + 1))
           << std::fixed << d10 << 'E' << e10;
        return os.str();
    }
#endif
};

template<std::floating_point M, std::integral E>
inline FloatExp<M, E> operator *(M a, const FloatExp<M, E>& b) noexcept {
    return FloatExp<M, E>(a) * b;
}

template<std::floating_point M, std::integral E>
inline FloatExp<M, E> operator *(const FloatExp<M, E>& b, M a) noexcept {
    return b * FloatExp<M, E>(a);
}

template<std::floating_point M, std::integral E>
inline FloatExp<M, E> operator /(M a, const FloatExp<M, E>& b) noexcept {
    return FloatExp<M, E>(a) / b;
}

template<std::floating_point M, std::integral E>
inline FloatExp<M, E> operator /(const FloatExp<M, E>& b, M a) noexcept {
    return b / FloatExp<M, E>(a);
}

template<std::floating_point M, std::integral E>
inline FloatExp<M, E> operator +(M a, const FloatExp<M, E>& b) noexcept {
    return FloatExp<M, E>(a) + b;
}

template<std::floating_point M, std::integral E>
inline FloatExp<M, E> operator +(const FloatExp<M, E>& b, M a) noexcept {
    return FloatExp<M, E>(a) + b;
}

template<std::floating_point M, std::integral E>
inline FloatExp<M, E> operator -(M a, const FloatExp<M, E>& b) noexcept {
    return FloatExp<M, E>(a) - b;
}

template<std::floating_point M, std::integral E>
inline FloatExp<M, E> operator -(const FloatExp<M, E>& b, M a) noexcept {
    return b - FloatExp<M, E>(a);
}

#if !defined(__CUDACC__)
template<std::floating_point M, std::integral E>
inline std::ostream& operator <<(std::ostream& os, const FloatExp<M, E>& b) noexcept {
    return os << b.toString();
}
#endif

template<std::floating_point M, std::integral E>
WF_HD
inline FloatExp<M, E> abs(FloatExp<M, E> a) noexcept {
    return a.val < 0 ? -a : a;
}

template<std::floating_point M, std::integral E>
WF_HD
inline FloatExp<M, E> sqrt(FloatExp<M, E> a) noexcept {
    return FloatExp<M, E>(
        WF_STD::sqrt((a.exp & 1) ? M(2) * a.val : a.val),
        (a.exp & 1) ? (a.exp - 1) / 2 : a.exp / 2
    );
}

template<std::floating_point M, std::integral E>
WF_HD
inline FloatExp<M, E> sqr(FloatExp<M, E> a) noexcept {
    return a * a;
}

template<typename T>
WF_HD
inline T pow(T x, uint64_t n) noexcept {
    switch (n) {
        case 0: return T(1);
        case 1: return x;
        case 2: return x * x;
        case 3: return x * x * x;
        case 4: { T t = x * x; return t * t; }
        case 5: { T t = x * x; return t * t * x; }
        case 6: { T t = x * x; return t * t * t; }
        case 7: { T t = x * x; return t * t * t * x; }
        case 8: { T t = x * x; t = t * t; return t * t; }
        default: {
            T y(1);
            while (n > 1) {
                if (n & 1) y *= x;
                x = x * x;
                n >>= 1;
            }
            return x * y;
        }
    }
}

template<std::floating_point M, std::integral E>
WF_HD
inline bool isnan(const FloatExp<M, E>& a) noexcept {
    return WF_STD::isnan(a.val);
}

template<std::floating_point M, std::integral E>
WF_HD
inline bool isinf(const FloatExp<M, E>& a) noexcept {
    return WF_STD::isinf(a.val);
}

template<std::floating_point M, std::integral E>
WF_HD
inline FloatExp<M, E> infnan_to_zero(const FloatExp<M, E>& a) noexcept {
    using WF_STD::copysign;
    return WF_STD::isinf(a.val) ? FloatExp<M, E>(copysign(M(1e30), a.val))
         : WF_STD::isnan(a.val) ? FloatExp<M, E>(M(0))
         : a;
}

template<std::floating_point M, std::integral E>
WF_HD
inline FloatExp<M, E> exp(FloatExp<M, E> a) noexcept {
    using WF_STD::exp;
    using WF_STD::ldexp;
    constexpr E SMALL = sizeof(M) == 8 ? -53 : -24;
    constexpr E LARGE = sizeof(M) == 8 ?   8 :   4;
    constexpr E OVERFLOW = sizeof(M) == 8 ?  61 :  30;
    if (SMALL <= a.exp && a.exp <= LARGE)
        return FloatExp<M, E>(exp(ldexp(a.val, int(a.exp))));
    if (a.exp >= OVERFLOW)
        return FloatExp<M, E>(a.val > M(0) ? M(1) / M(0) : M(0));
    if (a.exp < SMALL)
        return FloatExp<M, E>(M(1));
    return pow(FloatExp<M, E>(exp(a.val)), uint64_t(1) << a.exp);
}

template<std::floating_point M, std::integral E>
WF_HD
inline FloatExp<M, E> expm1(FloatExp<M, E> a) noexcept {
    using WF_STD::expm1;
    using WF_STD::ldexp;
    if (a.exp <= -120) return a;
    if (a.exp >= 8) return exp(a) - FloatExp<M, E>(1);
    return FloatExp<M, E>(expm1(ldexp(a.val, int(a.exp))));
}

template<std::floating_point M, std::integral E>
WF_HD
inline FloatExp<M, E> sin(FloatExp<M, E> a) noexcept {
    using WF_STD::sin;
    using WF_STD::ldexp;
    if (a.exp <= -120) return a;
    return FloatExp<M, E>(sin(ldexp(a.val, int(a.exp))));
}

template<std::floating_point M, std::integral E>
WF_HD
inline FloatExp<M, E> cos(FloatExp<M, E> a) noexcept {
    using WF_STD::cos;
    using WF_STD::ldexp;
    if (a.exp <= -120) return FloatExp<M, E>(M(1));
    return FloatExp<M, E>(cos(ldexp(a.val, int(a.exp))));
}

template<std::floating_point M, std::integral E>
WF_HD
inline FloatExp<M, E> log(FloatExp<M, E> a) noexcept {
    using WF_STD::log;
    return FloatExp<M, E>(log(a.val) + log(M(2)) * E(a.exp));
}

template<std::floating_point M, std::integral E>
WF_HD
inline FloatExp<M, E> log2(FloatExp<M, E> a) noexcept {
    using WF_STD::log2;
    return FloatExp<M, E>(log2(a.val) + a.exp);
}

template<std::floating_point M, std::integral E>
WF_HD
inline FloatExp<M, E> log1p(FloatExp<M, E> a) noexcept {
    using WF_STD::log1p;
    constexpr E THRESHOLD = sizeof(M) == 8 ? 53 : 24;
    if (a.exp < -THRESHOLD)
        return a;
    else if (a.exp > THRESHOLD)
        return log(FloatExp<M, E>(1) + a);
    else
        return FloatExp<M, E>(log1p(M(a)));
}

template<std::floating_point M, std::integral E>
WF_HD
inline FloatExp<M, E> diffabs(const FloatExp<M, E>& c, const FloatExp<M, E>& d) noexcept {
    const FloatExp<M, E> cd = c + d;
    const FloatExp<M, E> c2d = c.mul2() + d;
    return c.val >= M(0) ? cd.val >= M(0) ? d : -c2d
                        : cd.val >  M(0) ? c2d : -d;
}

} // namespace wacfrac

#if !defined(__CUDACC__) && defined(__cpp_lib_format)
template<std::floating_point M, std::integral E>
struct std::formatter<wacfrac::FloatExp<M, E>> : std::formatter<double> {
    auto format(const wacfrac::FloatExp<M, E>& v, std::format_context& ctx) const {
        return std::formatter<double>::format(static_cast<double>(v), ctx);
    }
};
#endif
