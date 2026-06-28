#pragma once

#include <memory>
#include <boost/multiprecision/fwd.hpp>
#include <algorithm>
#include <concepts>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ios>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <sstream>
#include <tuple>
#include <type_traits>

namespace wacfrac {

template<std::floating_point M, std::integral E>
struct FloatExp {
    using signed_types = std::tuple<std::int8_t, std::int16_t, std::int32_t, std::int64_t, std::intmax_t>;
    using unsigned_types = std::tuple<std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t, std::uintmax_t>;
    using float_types = std::tuple<float, double, long double>;
    using exponent_type = E;

    M mantissa;
    E exponent;

    FloatExp() : mantissa(0), exponent(0) {}
    FloatExp(const FloatExp&) = default;
    FloatExp& operator=(const FloatExp&) = default;

    FloatExp(float a) : mantissa(0), exponent(0) { *this = a; }
    FloatExp(double a) : mantissa(0), exponent(0) { *this = a; }
    FloatExp(long double a) : mantissa(0), exponent(0) { *this = a; }
    FloatExp(std::int8_t a) : mantissa(0), exponent(0) { *this = a; }
    FloatExp(std::int16_t a) : mantissa(0), exponent(0) { *this = a; }
    FloatExp(std::int32_t a) : mantissa(0), exponent(0) { *this = a; }
    FloatExp(std::int64_t a) : mantissa(0), exponent(0) { *this = a; }
    FloatExp(std::uint8_t a) : mantissa(0), exponent(0) { *this = a; }
    FloatExp(std::uint16_t a) : mantissa(0), exponent(0) { *this = a; }
    FloatExp(std::uint32_t a) : mantissa(0), exponent(0) { *this = a; }
    FloatExp(std::uint64_t a) : mantissa(0), exponent(0) { *this = a; }
    FloatExp(const char* s) : mantissa(0), exponent(0) { *this = s; }

    FloatExp& operator=(float a);
    FloatExp& operator=(double a);
    FloatExp& operator=(long double a);
    FloatExp& operator=(std::int8_t a);
    FloatExp& operator=(std::int16_t a);
    FloatExp& operator=(std::int32_t a);
    FloatExp& operator=(std::int64_t a);
    FloatExp& operator=(std::uint8_t a);
    FloatExp& operator=(std::uint16_t a);
    FloatExp& operator=(std::uint32_t a);
    FloatExp& operator=(std::uint64_t a);
    FloatExp& operator=(const char* s);

    int compare(const FloatExp& other) const noexcept;

    int compare(float a) const;
    int compare(double a) const;
    int compare(long double a) const;
    int compare(std::int8_t a) const;
    int compare(std::int16_t a) const;
    int compare(std::int32_t a) const;
    int compare(std::int64_t a) const;
    int compare(std::uint8_t a) const;
    int compare(std::uint16_t a) const;
    int compare(std::uint32_t a) const;
    int compare(std::uint64_t a) const;

    void swap(FloatExp& other) noexcept;
    std::string str(std::streamsize ss, std::ios_base::fmtflags ff) const;
    void negate();


    FloatExp& operator+=(const FloatExp& o);
    FloatExp& operator-=(const FloatExp& o);
    FloatExp& operator*=(const FloatExp& o);
    FloatExp& operator/=(const FloatExp& o);

    FloatExp& operator++();
    FloatExp operator++(int);
    FloatExp& operator--();
    FloatExp operator--(int);

    FloatExp operator-() const;
};

// Implementation helpers

namespace detail {

// IEEE 754 bit-manipulation
template<std::floating_point M>
M frexp_fast(M x, int* e) {
    if constexpr (std::is_same_v<M, double>) {
        uint64_t bits = std::bit_cast<uint64_t>(x);
        int biased_e = static_cast<int>((bits >> 52) & 0x7FF);
        if (biased_e == 0) {
            return (bits & 0x7FFFFFFFFFFFFFFFULL) == 0 ? M(0) : std::frexp(x, e);
        }
        *e = biased_e - 1022;
        bits = (bits & 0x800FFFFFFFFFFFFFULL) | (static_cast<uint64_t>(1022) << 52);
        return std::bit_cast<double>(bits);
    } else if constexpr (std::is_same_v<M, float>) {
        uint32_t bits = std::bit_cast<uint32_t>(x);
        int biased_e = static_cast<int>((bits >> 23) & 0xFF);
        if (biased_e == 0) {
            return (bits & 0x7FFFFFFF) == 0 ? M(0) : std::frexp(x, e);
        }
        *e = biased_e - 126;
        bits = (bits & 0x807FFFFF) | (126u << 23);
        return std::bit_cast<float>(bits);
    } else {
        return std::frexp(x, e);
    }
}

// Fast x * 2^k via exponent manipulation (normal range only)
template<std::floating_point M>
M ldexp_fast(M x, int k) {
    if (x == M(0)) return M(0);
    if constexpr (std::is_same_v<M, double>) {
        uint64_t bits = std::bit_cast<uint64_t>(x);
        int64_t be = static_cast<int64_t>((bits >> 52) & 0x7FF);
        int64_t nbe = be + k;
        if (nbe >= 0x7FF) {
            bits = (bits & 0x8000000000000000ULL) | 0x7FF0000000000000ULL;
        } else if (nbe < 1) {
            return std::ldexp(x, k);
        } else {
            bits = (bits & 0x800FFFFFFFFFFFFFULL) | (static_cast<uint64_t>(nbe) << 52);
        }
        return std::bit_cast<double>(bits);
    } else if constexpr (std::is_same_v<M, float>) {
        uint32_t bits = std::bit_cast<uint32_t>(x);
        int64_t be = static_cast<int64_t>((bits >> 23) & 0xFF);
        int64_t nbe = be + k;
        if (nbe >= 0xFF) {
            bits = (bits & 0x80000000) | 0x7F800000;
        } else if (nbe < 1) {
            return std::ldexp(x, k);
        } else {
            bits = (bits & 0x807FFFFF) | (static_cast<uint32_t>(nbe) << 23);
        }
        return std::bit_cast<float>(bits);
    } else {
        return std::ldexp(x, k);
    }
}

template<std::floating_point M, std::integral E>
void normalize(FloatExp<M, E>& x) {
    if (x.mantissa == 0) {
        x.exponent = 0;
        return;
    }
    M m = std::abs(x.mantissa);
    if (m >= M(0.5) && m < M(1.0)) return;
    int e = 0;
    x.mantissa = frexp_fast(x.mantissa, &e);
    x.exponent += e;
}

} // namespace detail

#define APPLY_X_TO_TYPES \
    X(float) \
    X(double) \
    X(long double) \
    X(std::int8_t) \
    X(std::int16_t) \
    X(std::int32_t) \
    X(std::int64_t) \
    X(std::uint8_t) \
    X(std::uint16_t) \
    X(std::uint32_t) \
    X(std::uint64_t)

// Assignment operators

#define X(T)                                            \
    template<std::floating_point M, std::integral E>              \
    inline FloatExp<M, E>&                               \
    FloatExp<M, E>::operator=(T a) {                     \
        if (a == 0) { mantissa = 0; exponent = 0; return *this; } \
        int e = 0;                                                \
        mantissa = detail::frexp_fast(static_cast<M>(a), &e);     \
        exponent = e;                                             \
        return *this;                                             \
    }
APPLY_X_TO_TYPES
#undef X

// Comparison

template<std::floating_point M, std::integral E>
inline int FloatExp<M, E>::compare(const FloatExp& o) const noexcept {
    // Zero cases
    if (mantissa == 0 && o.mantissa == 0) return 0;
    if (mantissa == 0) return o.mantissa > 0 ? -1 : 1;
    if (o.mantissa == 0) return mantissa > 0 ? 1 : -1;

    // Sign cases
    if (mantissa > 0 && o.mantissa < 0) return 1;
    if (mantissa < 0 && o.mantissa > 0) return -1;

    bool pos = mantissa > 0;
    E exp_diff = exponent - o.exponent;

    // Check if exponent difference too large to overcome
    constexpr int prec = std::numeric_limits<M>::digits;
    if (exp_diff > prec + 1) return pos ? 1 : -1;
    if (exp_diff < -prec - 1) return pos ? -1 : 1;

    // Compare aligned mantissas
    long double a = static_cast<long double>(mantissa);
    long double b = static_cast<long double>(o.mantissa);

    if (exp_diff > 0)
        b = std::ldexp(b, -static_cast<int>(exp_diff));
    else if (exp_diff < 0)
        a = std::ldexp(a, static_cast<int>(exp_diff));

    int cmp = (a > b) - (a < b);
    return pos ? cmp : -cmp;
}

#define X(T) \
    template<std::floating_point M, std::integral E> \
    inline int FloatExp<M, E>::compare(T a) const { \
        FloatExp t; \
        t.mantissa = 0; t.exponent = 0; \
        if (a != 0) { int e = 0; t.mantissa = detail::frexp_fast(static_cast<M>(a), &e); t.exponent = e; } \
        return compare(t);                                                       \
    }
APPLY_X_TO_TYPES
#undef X
#undef APPLY_X_TO_TYPES

// String assignment

template<std::floating_point M, std::integral E>
inline FloatExp<M, E>&
FloatExp<M, E>::operator=(const char* s) {
    char* end;
    long double val = std::strtold(s, &end);
    if (end == s) throw std::runtime_error("Invalid number string");
    *this = val;
    return *this;
}

// swap / str / negate

template<std::floating_point M, std::integral E>
inline void FloatExp<M, E>::swap(FloatExp& other) noexcept {
    std::swap(mantissa, other.mantissa);
    std::swap(exponent, other.exponent);
}

template<std::floating_point M, std::integral E>
inline std::string FloatExp<M, E>::str(std::streamsize ss, std::ios_base::fmtflags ff) const {
    long double v = static_cast<long double>(mantissa);
    if (exponent >= std::numeric_limits<int>::min() && exponent <= std::numeric_limits<int>::max())
        v = std::ldexp(v, static_cast<int>(exponent));
    else if (exponent > 0)
        v = v * std::exp2(static_cast<long double>(exponent));
    else
        v = v / std::exp2(static_cast<long double>(-exponent));

    std::ostringstream oss;
    oss.flags(ff);
    if (ss > 0) oss.precision(ss);
    oss << v;
    return oss.str();
}

template<std::floating_point M, std::integral E>
inline void FloatExp<M, E>::negate() {
    mantissa = -mantissa;
}

// Compound assignment

template<std::floating_point M, std::integral E>
inline FloatExp<M, E>&
FloatExp<M, E>::operator+=(const FloatExp& o) {
    if (o.mantissa == 0) return *this;
    if (mantissa == 0) { mantissa = o.mantissa; exponent = o.exponent; return *this; }

    // Align exponents
    E exp_diff = exponent - o.exponent;
    if (exp_diff == 0) {
        mantissa += o.mantissa;
    } else if (exp_diff > 0) {
        int sh = static_cast<int>(std::min(exp_diff, static_cast<E>(std::numeric_limits<int>::max())));
        mantissa += detail::ldexp_fast(o.mantissa, -sh);
    } else {
        int sh = static_cast<int>(std::max(exp_diff, static_cast<E>(std::numeric_limits<int>::min())));
        mantissa = detail::ldexp_fast(mantissa, sh) + o.mantissa;
        exponent = o.exponent;
    }

    detail::normalize(*this);
    return *this;
}

template<std::floating_point M, std::integral E>
inline FloatExp<M, E>&
FloatExp<M, E>::operator-=(const FloatExp& o) {
    if (o.mantissa == 0) return *this;
    if (mantissa == 0) { mantissa = -o.mantissa; exponent = o.exponent; return *this; }

    E exp_diff = exponent - o.exponent;
    if (exp_diff == 0) {
        mantissa -= o.mantissa;
    } else if (exp_diff > 0) {
        int sh = static_cast<int>(std::min(exp_diff, static_cast<E>(std::numeric_limits<int>::max())));
        mantissa -= detail::ldexp_fast(o.mantissa, -sh);
    } else {
        int sh = static_cast<int>(std::max(exp_diff, static_cast<E>(std::numeric_limits<int>::min())));
        mantissa = detail::ldexp_fast(mantissa, sh) - o.mantissa;
        exponent = o.exponent;
    }

    detail::normalize(*this);
    return *this;
}

template<std::floating_point M, std::integral E>
inline FloatExp<M, E>&
FloatExp<M, E>::operator*=(const FloatExp& o) {
    if (mantissa == 0) return *this;
    if (o.mantissa == 0) { mantissa = 0; exponent = 0; return *this; }
    mantissa *= o.mantissa;
    exponent += o.exponent;
    detail::normalize(*this);
    return *this;
}

template<std::floating_point M, std::integral E>
inline FloatExp<M, E>&
FloatExp<M, E>::operator/=(const FloatExp& o) {
    if (o.mantissa == 0) throw std::overflow_error("Division by zero");
    if (mantissa == 0) return *this;
    mantissa /= o.mantissa;
    exponent -= o.exponent;
    detail::normalize(*this);
    return *this;
}

//  Increment / Decrement
template<std::floating_point M, std::integral E>
inline FloatExp<M, E>&
FloatExp<M, E>::operator++() {
    *this += FloatExp(1.0L);
    return *this;
}

template<std::floating_point M, std::integral E>
inline FloatExp<M, E>
FloatExp<M, E>::operator++(int) {
    FloatExp tmp(*this);
    ++*this;
    return tmp;
}

template<std::floating_point M, std::integral E>
inline FloatExp<M, E>&
FloatExp<M, E>::operator--() {
    *this -= FloatExp(1.0L);
    return *this;
}

template<std::floating_point M, std::integral E>
inline FloatExp<M, E>
FloatExp<M, E>::operator--(int) {
    FloatExp tmp(*this);
    --*this;
    return tmp;
}

// Unary minus
template<std::floating_point M, std::integral E>
inline FloatExp<M, E>
FloatExp<M, E>::operator-() const {
    FloatExp r(*this);
    r.negate();
    return r;
}

} // namespace wacfrac


// eval_* functions (Boost.Multiprecision backend)

namespace wacfrac {

// Arithmetic

// 2-arg
template<std::floating_point M, std::integral E>
inline void eval_add(FloatExp<M, E>& b, const FloatExp<M, E>& cb) {
    b += cb;
}
template<std::floating_point M, std::integral E>
inline void eval_subtract(FloatExp<M, E>& b, const FloatExp<M, E>& cb) {
    b -= cb;
}
template<std::floating_point M, std::integral E>
inline void eval_multiply(FloatExp<M, E>& b, const FloatExp<M, E>& cb) {
    b *= cb;
}
template<std::floating_point M, std::integral E>
inline void eval_divide(FloatExp<M, E>& b, const FloatExp<M, E>& cb) {
    b /= cb;
}

// 3-arg: result = a op b
template<std::floating_point M, std::integral E>
inline void eval_add(FloatExp<M, E>& result, const FloatExp<M, E>& a, const FloatExp<M, E>& b) {
    result = a; eval_add(result, b);
}
template<std::floating_point M, std::integral E>
inline void eval_subtract(FloatExp<M, E>& result, const FloatExp<M, E>& a, const FloatExp<M, E>& b) {
    result = a; eval_subtract(result, b);
}
template<std::floating_point M, std::integral E>
inline void eval_multiply(FloatExp<M, E>& result, const FloatExp<M, E>& a, const FloatExp<M, E>& b) {
    result = a; eval_multiply(result, b);
}
template<std::floating_point M, std::integral E>
inline void eval_divide(FloatExp<M, E>& result, const FloatExp<M, E>& a, const FloatExp<M, E>& b) {
    result = a; eval_divide(result, b);
}

// Conversion

namespace detail {

template<std::floating_point M, std::integral E, typename T>
void convert_to_impl(T* pa, const FloatExp<M, E>& cb) {
    T r = static_cast<T>(cb.mantissa);
    E e = cb.exponent;
    while (e > 0) {
        int s = std::min(e, static_cast<E>(std::numeric_limits<int>::max()));
        r = std::ldexp(r, s);
        e -= s;
    }
    while (e < 0) {
        int s = std::min(-e, static_cast<E>(std::numeric_limits<int>::max()));
        r = std::ldexp(r, -s);
        e += s;
    }
    *pa = r;
}

} // namespace detail

template<std::floating_point M, std::integral E, std::floating_point T>
inline void eval_convert_to(T* pa, const FloatExp<M, E>& cb) {
    detail::convert_to_impl(pa, cb);
}

template<std::floating_point M, std::integral E, std::integral T>
inline void eval_convert_to(T* pa, const FloatExp<M, E>& cb) {
    long double tmp;
    detail::convert_to_impl(&tmp, cb);
    *pa = static_cast<T>(tmp);
}

// Frexp
template<std::floating_point M, std::integral E>
inline void eval_frexp(FloatExp<M, E>& b, const FloatExp<M, E>& cb, E* pexp) {
    if (cb.mantissa == 0) {
        b.mantissa = 0;
        b.exponent = 0;
        *pexp = 0;
        return;
    }
    int extra = 0;
    b.mantissa = detail::frexp_fast(cb.mantissa, &extra);
    b.exponent = 0;
    *pexp = cb.exponent + extra;
}

template<std::floating_point M, std::integral E>
inline void eval_frexp(FloatExp<M, E>& b, const FloatExp<M, E>& cb, int* pi) {
    if (cb.mantissa == 0) {
        b.mantissa = 0;
        b.exponent = 0;
        *pi = 0;
        return;
    }
    int extra = 0;
    b.mantissa = detail::frexp_fast(cb.mantissa, &extra);
    b.exponent = 0;
    long long sum = static_cast<long long>(cb.exponent) + extra;
    if (sum < std::numeric_limits<int>::min() || sum > std::numeric_limits<int>::max())
        throw std::runtime_error("Exponent too large for int");
    *pi = static_cast<int>(sum);
}

// Ldexp
template<std::floating_point M, std::integral E>
inline void eval_ldexp(FloatExp<M, E>& b, const FloatExp<M, E>& cb, E exp) {
    b = cb;
    b.exponent += exp;
}

template<std::floating_point M, std::integral E>
inline void eval_ldexp(FloatExp<M, E>& b, const FloatExp<M, E>& cb, int i) {
    b = cb;
    b.exponent += i;
}

// Floor
template<std::floating_point M, std::integral E>
inline void eval_floor(FloatExp<M, E>& b, const FloatExp<M, E>& cb) {
    if (cb.mantissa == 0) { b.mantissa = 0; b.exponent = 0; return; }
    long double v;
    eval_convert_to(&v, cb);
    long double f = std::floor(v);
    b = f;
}

// Ceil
template<std::floating_point M, std::integral E>
inline void eval_ceil(FloatExp<M, E>& b, const FloatExp<M, E>& cb) {
    if (cb.mantissa == 0) { b.mantissa = 0; b.exponent = 0; return; }
    long double v;
    eval_convert_to(&v, cb);
    long double c = std::ceil(v);
    b = c;
}

// Sqrt
template<std::floating_point M, std::integral E>
inline void eval_sqrt(FloatExp<M, E>& b, const FloatExp<M, E>& cb) {
    if (cb.mantissa < 0) throw std::domain_error("sqrt of negative number");
    if (cb.mantissa == 0) { b.mantissa = 0; b.exponent = 0; return; }

    // sqrt(mantissa * 2^exponent) = sqrt(mantissa) * 2^(exponent/2)
    b = cb;
    if (b.exponent % 2 != 0) {
        b.mantissa *= static_cast<M>(2.0L);
        b.exponent -= 1;
    }
    b.mantissa = std::sqrt(b.mantissa);
    b.exponent /= 2;
    detail::normalize(b);
}

// Log10
template<std::floating_point M, std::integral E>
inline void eval_log10(FloatExp<M, E>& b, const FloatExp<M, E>& cb) {
    if (cb.mantissa == M(0)) {
        throw std::overflow_error("log10 of zero");
    }
    if (cb.mantissa < M(0)) {
        throw std::domain_error("log10 of negative number");
    }
    b.mantissa = static_cast<M>(std::log10(cb.mantissa)
        + static_cast<long double>(cb.exponent) * std::log10(2.0L));
    b.exponent = 0;
    detail::normalize(b);
}

// Classification / sign / abs

template<std::floating_point M, std::integral E>
inline bool eval_is_zero(const FloatExp<M, E>& x) {
    return x.mantissa == M(0);
}

template<std::floating_point M, std::integral E>
inline int eval_get_sign(const FloatExp<M, E>& x) {
    return (x.mantissa > M(0)) - (x.mantissa < M(0));
}

template<std::floating_point M, std::integral E>
inline void eval_fabs(FloatExp<M, E>& result, const FloatExp<M, E>& x) {
    result = x;
    if (result.mantissa < M(0))
        result.mantissa = -result.mantissa;
}

template<std::floating_point M, std::integral E>
inline int eval_signbit(const FloatExp<M, E>& x) {
    return std::signbit(x.mantissa);
}

template<std::floating_point M, std::integral E>
inline int eval_fpclassify(const FloatExp<M, E>& x) {
    if (x.mantissa == M(0))
        return FP_ZERO;
    if (std::isnan(x.mantissa))
        return FP_NAN;
    if (std::isinf(x.mantissa))
        return FP_INFINITE;
    return FP_NORMAL;
}

// Binary operators
#define APPLY_X_TO_OPS \
    X(+) \
    X(-) \
    X(*) \
    X(/)

#define X(OP) \
    template<std::floating_point M, std::integral E> \
    inline FloatExp<M, E> \
    operator OP (const FloatExp<M, E>& a, const FloatExp<M, E>& b) { \
        FloatExp<M, E> r(a); \
        r OP##= b; \
        return r; \
    }
APPLY_X_TO_OPS
#undef X
#undef APPLY_X_TO_OPS

// Comparison operators

#define APPLY_X_TO_OPS \
    X(==) \
    X(!=) \
    X(<) \
    X(<=) \
    X(>) \
    X(>=)

#define X(OP) \
    template<std::floating_point M, std::integral E> \
    inline bool operator OP (const FloatExp<M, E>& a, const FloatExp<M, E>& b) { \
        return a.compare(b) OP 0; \
    }
APPLY_X_TO_OPS
#undef X
#undef APPLY_X_TO_OPS

// Stream operators

template<std::floating_point M, std::integral E>
inline std::ostream& operator<<(std::ostream& os, const FloatExp<M, E>& v) {
    os << v.str(os.precision(), os.flags());
    return os;
}

template<std::floating_point M, std::integral E>
inline std::istream& operator>>(std::istream& is, FloatExp<M, E>& v) {
    std::string s;
    is >> s;
    v = s.c_str();
    return is;
}

} // namespace wacfrac

namespace boost::multiprecision {

template <class Backend>
struct number_category;

template<std::floating_point M, std::integral E>
struct number_category<wacfrac::FloatExp<M, E>>
    : std::integral_constant<int, 1> {};

} // namespace boost::multiprecision

namespace std {

template <std::floating_point M, std::integral E, boost::multiprecision::expression_template_option ET>
class numeric_limits<boost::multiprecision::number<wacfrac::FloatExp<M, E>, ET>>
{
    using number_type = boost::multiprecision::number<wacfrac::FloatExp<M, E>, ET>;

public:
    static constexpr bool is_specialized = true;
    static constexpr int digits       = std::numeric_limits<M>::digits;
    static constexpr int digits10     = std::numeric_limits<M>::digits10;
    static constexpr int max_digits10 = std::numeric_limits<M>::max_digits10;
    static constexpr bool is_signed   = true;
    static constexpr bool is_integer  = false;
    static constexpr bool is_exact    = false;
    static constexpr int radix        = std::numeric_limits<M>::radix;

    static constexpr int min_exponent   = std::numeric_limits<M>::min_exponent;
    static constexpr int min_exponent10 = std::numeric_limits<M>::min_exponent10;
    static constexpr int max_exponent   = std::numeric_limits<M>::max_exponent;
    static constexpr int max_exponent10 = std::numeric_limits<M>::max_exponent10;

    static constexpr bool has_infinity      = false;
    static constexpr bool has_quiet_NaN     = false;
    static constexpr bool has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm = denorm_absent;
    static constexpr bool has_denorm_loss   = false;
    static constexpr bool is_iec559         = false;
    static constexpr bool is_bounded        = true;
    static constexpr bool is_modulo         = false;
    static constexpr bool traps             = false;
    static constexpr bool tinyness_before   = false;
    static constexpr float_round_style round_style = round_to_nearest;

    static constexpr number_type(min)()      noexcept { return number_type{std::numeric_limits<M>::min()}; }
    static constexpr number_type(max)()      noexcept { return number_type{std::numeric_limits<M>::max()}; }
    static constexpr number_type lowest()    noexcept { return number_type{-std::numeric_limits<M>::max()}; }
    static constexpr number_type epsilon()   noexcept { return number_type{std::numeric_limits<M>::epsilon()}; }
    static constexpr number_type round_error() noexcept { return number_type{0.5L}; }
    static constexpr number_type infinity()      noexcept { return number_type{0}; }
    static constexpr number_type quiet_NaN()     noexcept { return number_type{0}; }
    static constexpr number_type signaling_NaN() noexcept { return number_type{0}; }
    static constexpr number_type denorm_min()    noexcept { return number_type{0}; }
};

} // namespace std

namespace wacfrac {

// Aliases

using SingleExp = FloatExp<float, int64_t>;
using DoubleExp = FloatExp<double, int64_t>;
// using quadexp   = FloatExp<long double, int64_t>; Unused

} // namespace wacfrac
