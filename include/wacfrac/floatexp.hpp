#pragma once

#include <climits>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <limits>
#include <sycl/sycl.hpp>
#if defined(__cpp_impl_three_way_comparison) && __has_include(<compare>)
#include <compare>
#endif
#if defined(__cpp_lib_format)
#include <format>
#endif

namespace wacfrac
{

// Adapted from: https://mathr.co.uk/kf/kf.html

template <std::floating_point M, std::integral E> struct FloatExp {
    using mantissa_type = M;
    using exponent_type = E;

    M val;
    E exp;

    SYCL_EXTERNAL
    FloatExp() noexcept
        : val(0),
          exp(EXP_MIN)
    {
    }
    FloatExp(const FloatExp&) = default;
    FloatExp(FloatExp&&) = default;
    FloatExp& operator=(const FloatExp&) = default;
    FloatExp& operator=(FloatExp&&) = default;
    ~FloatExp() = default;

    static constexpr E EXPONENT_MASK = sizeof(M) == 8 ? 0x7FF0000000000000LL : 0x7F800000;
    static constexpr E EXPONENT_UNMASK = sizeof(M) == 8 ? 0x800FFFFFFFFFFFFFLL : 0x803FFFFF;
    static constexpr E EXPONENT_SET = sizeof(M) == 8 ? 0x3FF0000000000000LL : 0x3F800000;
    static constexpr int EXPONENT_SHIFT = sizeof(M) == 8 ? 52 : 23;
    static constexpr int EXPONENT_BIAS = sizeof(M) == 8 ? 1023 : 127;

    static constexpr E MAX_PREC = sizeof(M) == 8 ? 53 : 24;
    // MIN_EXPONENT is smaller than you might expect, this is to give headroom for
    // avoiding overflow in + and other functions. it is the exponent for 0.0
    static constexpr E EXP_MIN = sizeof(E) == 8 ? E(-0x800000000000000LL) : E(-0x8000000);
    static constexpr E EXP_MAX = -EXP_MIN;

private:
    SYCL_EXTERNAL
    inline void align() noexcept
    {
        if (val == 0) {
            exp = EXP_MIN;
            return;
        }
        E val_i;
        std::memcpy(&val_i, &val, sizeof(val_i));
        const E e = val_i & EXPONENT_MASK;

        if (e == 0) [[unlikely]] {
            val = 0;
            exp = EXP_MIN;
        } else {
            exp += (e >> EXPONENT_SHIFT) - EXPONENT_BIAS;
            val_i = (val_i & EXPONENT_UNMASK) | EXPONENT_SET;
            std::memcpy(&val, &val_i, sizeof(val));
        }
    }

public:
    SYCL_EXTERNAL
    static inline M set_exp(M val, E exp) noexcept
    {
        E val_i;
        std::memcpy(&val_i, &val, sizeof(val_i));
        val_i = (val_i & EXPONENT_UNMASK) | ((exp + EXPONENT_BIAS) << EXPONENT_SHIFT);
        std::memcpy(&val, &val_i, sizeof(val));
        return val;
    }

    template <std::integral I>
    SYCL_EXTERNAL FloatExp(I a) noexcept
        : val(a),
          exp(0)
    {
        align();
    }

    template <std::floating_point F> SYCL_EXTERNAL FloatExp(F a) noexcept
    {
        using std::frexp;
        auto e{0};
        a = frexp(a, &e);
        val = a;
        exp = e;
        align();
    }

    SYCL_EXTERNAL
    FloatExp(M a, E e) noexcept
        : val(a),
          exp(e)
    {
        align();
    }

    SYCL_EXTERNAL
    FloatExp(M a, E e, int) noexcept
        : val(a),
          exp(e)
    {
    }

    template <std::floating_point M2, std::integral E2> SYCL_EXTERNAL FloatExp(const FloatExp<M2, E2>& a) noexcept
    {
        using std::max;
        using std::min;
        E clamped = static_cast<E>(a.exp);
        clamped = max(clamped, E(EXP_MIN));
        clamped = min(clamped, E(EXP_MAX));
        val = static_cast<M>(a.val);
        exp = clamped;
        align();
    }

    SYCL_EXTERNAL
    inline FloatExp& operator*=(const FloatExp& a) noexcept
    {
        val *= a.val;
        exp += a.exp;
        align();
        return *this;
    }

    SYCL_EXTERNAL
    inline FloatExp& operator/=(const FloatExp& a) noexcept
    {
        val /= a.val;
        exp -= a.exp;
        align();
        return *this;
    }

    SYCL_EXTERNAL
    inline FloatExp& operator+=(const FloatExp& a) noexcept
    {
        *this = *this + a;
        return *this;
    }

    SYCL_EXTERNAL
    inline FloatExp& operator-=(const FloatExp& a) noexcept
    {
        *this = *this - a;
        return *this;
    }

    SYCL_EXTERNAL
    inline FloatExp operator*(const FloatExp& a) const noexcept { return FloatExp(*this) *= a; }

    SYCL_EXTERNAL
    inline FloatExp operator/(const FloatExp& a) const noexcept { return FloatExp(*this) /= a; }

    SYCL_EXTERNAL
    inline FloatExp operator+(const FloatExp& a) const noexcept
    {
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

    SYCL_EXTERNAL
    inline FloatExp operator-(const FloatExp& a) const noexcept
    {
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

    SYCL_EXTERNAL
    inline FloatExp operator-() const noexcept
    {
        FloatExp r = *this;
        r.val = -r.val;
        return r;
    }

    SYCL_EXTERNAL
    inline FloatExp operator+() const noexcept { return *this; }

private:
    SYCL_EXTERNAL
    inline int compare(const FloatExp& a) const noexcept
    {
        if (val > 0) {
            if (a.val < 0)
                return 1;
            if (exp > a.exp)
                return 1;
            else if (exp < a.exp)
                return -1;
            return (val > a.val) - (val < a.val);
        } else {
            if (a.val > 0)
                return -1;
            if (exp > a.exp)
                return -1;
            else if (exp < a.exp)
                return 1;
            return (val > a.val) - (val < a.val);
        }
    }

public:
    SYCL_EXTERNAL
    inline bool operator==(const FloatExp& a) const noexcept
    {
        using std::isinf;
        if (val != 0 && !isinf(val) && exp != a.exp)
            return false;
        return val == a.val;
    }

    SYCL_EXTERNAL
    inline bool operator>(const FloatExp& a) const noexcept { return compare(a) > 0; }

    SYCL_EXTERNAL
    inline bool operator<(const FloatExp& a) const noexcept { return compare(a) < 0; }

    SYCL_EXTERNAL
    inline bool operator>=(const FloatExp& a) const noexcept { return compare(a) >= 0; }

    SYCL_EXTERNAL
    inline bool operator<=(const FloatExp& a) const noexcept { return compare(a) <= 0; }

#if defined(__cpp_impl_three_way_comparison) && __has_include(<compare>)
    inline std::partial_ordering operator<=>(const FloatExp& a) const noexcept
    {
        using std::isnan;
        if (isnan(val) || isnan(a.val))
            return std::partial_ordering::unordered;
        int cmp = compare(a);
        if (cmp > 0)
            return std::partial_ordering::greater;
        if (cmp < 0)
            return std::partial_ordering::less;
        return std::partial_ordering::equivalent;
    }
#endif

    SYCL_EXTERNAL
    inline operator float() const noexcept
    {
        using sycl::ldexp;
        if (exp > E(INT_MAX))
            return static_cast<float>(val) / 0.0f;
        if (exp < E(INT_MIN))
            return static_cast<float>(val) * 0.0f;
        return ldexp(static_cast<float>(val), static_cast<int>(exp));
    }

    SYCL_EXTERNAL
    inline operator double() const noexcept
    {
        using sycl::ldexp;
        if (exp > E(INT_MAX))
            return static_cast<double>(val) / 0.0;
        if (exp < E(INT_MIN))
            return static_cast<double>(val) * 0.0;
        return ldexp(static_cast<double>(val), static_cast<int>(exp));
    }
};

template <std::floating_point M, std::integral E>
SYCL_EXTERNAL inline FloatExp<M, E> operator*(M a, const FloatExp<M, E>& b) noexcept
{
    return FloatExp<M, E>(a) * b;
}

template <std::floating_point M, std::integral E>
SYCL_EXTERNAL inline FloatExp<M, E> operator*(const FloatExp<M, E>& b, M a) noexcept
{
    return b * FloatExp<M, E>(a);
}

template <std::floating_point M, std::integral E>
SYCL_EXTERNAL inline FloatExp<M, E> operator/(M a, const FloatExp<M, E>& b) noexcept
{
    return FloatExp<M, E>(a) / b;
}

template <std::floating_point M, std::integral E>
SYCL_EXTERNAL inline FloatExp<M, E> operator/(const FloatExp<M, E>& b, M a) noexcept
{
    return b / FloatExp<M, E>(a);
}

template <std::floating_point M, std::integral E>
SYCL_EXTERNAL inline FloatExp<M, E> operator+(M a, const FloatExp<M, E>& b) noexcept
{
    return FloatExp<M, E>(a) + b;
}

template <std::floating_point M, std::integral E>
SYCL_EXTERNAL inline FloatExp<M, E> operator+(const FloatExp<M, E>& b, M a) noexcept
{
    return FloatExp<M, E>(a) + b;
}

template <std::floating_point M, std::integral E>
SYCL_EXTERNAL inline FloatExp<M, E> operator-(M a, const FloatExp<M, E>& b) noexcept
{
    return FloatExp<M, E>(a) - b;
}

template <std::floating_point M, std::integral E>
SYCL_EXTERNAL inline FloatExp<M, E> operator-(const FloatExp<M, E>& b, M a) noexcept
{
    return b - FloatExp<M, E>(a);
}

template <std::floating_point M, std::integral E> SYCL_EXTERNAL inline FloatExp<M, E> abs(FloatExp<M, E> a) noexcept
{
    return a.val < 0 ? -a : a;
}

template <std::floating_point M, std::integral E> SYCL_EXTERNAL inline FloatExp<M, E> sqrt(FloatExp<M, E> a) noexcept
{
    using std::sqrt;
    return FloatExp<M, E>(sqrt((a.exp & 1) ? M(2) * a.val : a.val), (a.exp & 1) ? (a.exp - 1) / 2 : a.exp / 2);
}

} // namespace wacfrac

#if defined(__cpp_lib_format)
template <std::floating_point M, std::integral E>
struct std::formatter<wacfrac::FloatExp<M, E>> : std::formatter<double> {
    auto format(const wacfrac::FloatExp<M, E>& v, std::format_context& ctx) const
    {
        return std::formatter<double>::format(static_cast<double>(v), ctx);
    }
};
#endif
