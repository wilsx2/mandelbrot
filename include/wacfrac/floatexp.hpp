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
struct floatexp {
    using signed_types = std::tuple<std::int8_t, std::int16_t, std::int32_t, std::int64_t, std::intmax_t>;
    using unsigned_types = std::tuple<std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t, std::uintmax_t>;
    using float_types = std::tuple<float, double, long double>;
    using exponent_type = E;

    M mantissa;
    E exponent;

    floatexp() : mantissa(0), exponent(0) {}
    floatexp(const floatexp&) = default;
    floatexp& operator=(const floatexp&) = default;

    floatexp(float a) : mantissa(0), exponent(0) { *this = a; }
    floatexp(double a) : mantissa(0), exponent(0) { *this = a; }
    floatexp(long double a) : mantissa(0), exponent(0) { *this = a; }
    floatexp(std::int8_t a) : mantissa(0), exponent(0) { *this = a; }
    floatexp(std::int16_t a) : mantissa(0), exponent(0) { *this = a; }
    floatexp(std::int32_t a) : mantissa(0), exponent(0) { *this = a; }
    floatexp(std::int64_t a) : mantissa(0), exponent(0) { *this = a; }
    floatexp(std::uint8_t a) : mantissa(0), exponent(0) { *this = a; }
    floatexp(std::uint16_t a) : mantissa(0), exponent(0) { *this = a; }
    floatexp(std::uint32_t a) : mantissa(0), exponent(0) { *this = a; }
    floatexp(std::uint64_t a) : mantissa(0), exponent(0) { *this = a; }
    floatexp(const char* s) : mantissa(0), exponent(0) { *this = s; }

    floatexp& operator=(float a);
    floatexp& operator=(double a);
    floatexp& operator=(long double a);
    floatexp& operator=(std::int8_t a);
    floatexp& operator=(std::int16_t a);
    floatexp& operator=(std::int32_t a);
    floatexp& operator=(std::int64_t a);
    floatexp& operator=(std::uint8_t a);
    floatexp& operator=(std::uint16_t a);
    floatexp& operator=(std::uint32_t a);
    floatexp& operator=(std::uint64_t a);
    floatexp& operator=(const char* s);

    int compare(const floatexp& other) const noexcept;

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

    void swap(floatexp& other) noexcept;
    std::string str(std::streamsize ss, std::ios_base::fmtflags ff) const;
    void negate();


    floatexp& operator+=(const floatexp& o);
    floatexp& operator-=(const floatexp& o);
    floatexp& operator*=(const floatexp& o);
    floatexp& operator/=(const floatexp& o);

    floatexp& operator++();
    floatexp operator++(int);
    floatexp& operator--();
    floatexp operator--(int);

    floatexp operator-() const;
};

// Implementation helpers

namespace detail {

template<std::floating_point M, std::integral E>
void normalize(floatexp<M, E>& x) {
    if (x.mantissa == 0) {
        x.exponent = 0;
        return;
    }
    int e;
    x.mantissa = std::frexp(x.mantissa, &e);
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
    inline floatexp<M, E>&                               \
    floatexp<M, E>::operator=(T a) {                     \
        if (a == 0) { mantissa = 0; exponent = 0; return *this; } \
        int e;                                                    \
        mantissa = std::frexp(static_cast<M>(a), &e);             \
        exponent = e;                                             \
        return *this;                                             \
    }
APPLY_X_TO_TYPES
#undef X

// Comparison
template<std::floating_point M, std::integral E>
inline int floatexp<M, E>::compare(const floatexp& o) const noexcept {
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

#define X(T)                                                \
    template<std::floating_point M, std::integral E>                             \
    inline int floatexp<M, E>::compare(T a) const {                    \
        floatexp t;                                                     \
        t.mantissa = 0; t.exponent = 0;                                          \
        if (a != 0) { int e; t.mantissa = std::frexp(static_cast<M>(a), &e); t.exponent = e; } \
        return compare(t);                                                       \
    }
APPLY_X_TO_TYPES
#undef X
#undef APPLY_X_TO_TYPES

// String assignment

template<std::floating_point M, std::integral E>
inline floatexp<M, E>&
floatexp<M, E>::operator=(const char* s) {
    char* end;
    long double val = std::strtold(s, &end);
    if (end == s) throw std::runtime_error("Invalid number string");
    *this = val;
    return *this;
}

// swap / str / negate

template<std::floating_point M, std::integral E>
inline void floatexp<M, E>::swap(floatexp& other) noexcept {
    std::swap(mantissa, other.mantissa);
    std::swap(exponent, other.exponent);
}

template<std::floating_point M, std::integral E>
inline std::string floatexp<M, E>::str(std::streamsize ss, std::ios_base::fmtflags ff) const {
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
inline void floatexp<M, E>::negate() {
    mantissa = -mantissa;
}

// Compound assignment

template<std::floating_point M, std::integral E>
inline floatexp<M, E>&
floatexp<M, E>::operator+=(const floatexp& o) {
    if (o.mantissa == 0) return *this;
    if (mantissa == 0) { mantissa = o.mantissa; exponent = o.exponent; return *this; }

    // Align exponents
    E exp_diff = exponent - o.exponent;
    if (exp_diff == 0) {
        mantissa += o.mantissa;
    } else if (exp_diff > 0) {
        int sh = static_cast<int>(std::min(exp_diff, static_cast<E>(std::numeric_limits<int>::max())));
        mantissa += std::ldexp(o.mantissa, -sh);
    } else {
        int sh = static_cast<int>(std::max(exp_diff, static_cast<E>(std::numeric_limits<int>::min())));
        mantissa = std::ldexp(mantissa, sh) + o.mantissa;
        exponent = o.exponent;
    }

    detail::normalize(*this);
    return *this;
}

template<std::floating_point M, std::integral E>
inline floatexp<M, E>&
floatexp<M, E>::operator-=(const floatexp& o) {
    if (o.mantissa == 0) return *this;
    floatexp neg = o;
    neg.negate();
    return *this += neg;
}

template<std::floating_point M, std::integral E>
inline floatexp<M, E>&
floatexp<M, E>::operator*=(const floatexp& o) {
    if (mantissa == 0) return *this;
    if (o.mantissa == 0) { mantissa = 0; exponent = 0; return *this; }
    mantissa *= o.mantissa;
    exponent += o.exponent;
    detail::normalize(*this);
    return *this;
}

template<std::floating_point M, std::integral E>
inline floatexp<M, E>&
floatexp<M, E>::operator/=(const floatexp& o) {
    if (o.mantissa == 0) throw std::overflow_error("Division by zero");
    if (mantissa == 0) return *this;
    mantissa /= o.mantissa;
    exponent -= o.exponent;
    detail::normalize(*this);
    return *this;
}

//  Increment / Decrement
template<std::floating_point M, std::integral E>
inline floatexp<M, E>&
floatexp<M, E>::operator++() {
    *this += floatexp(1.0L);
    return *this;
}

template<std::floating_point M, std::integral E>
inline floatexp<M, E>
floatexp<M, E>::operator++(int) {
    floatexp tmp(*this);
    ++*this;
    return tmp;
}

template<std::floating_point M, std::integral E>
inline floatexp<M, E>&
floatexp<M, E>::operator--() {
    *this -= floatexp(1.0L);
    return *this;
}

template<std::floating_point M, std::integral E>
inline floatexp<M, E>
floatexp<M, E>::operator--(int) {
    floatexp tmp(*this);
    --*this;
    return tmp;
}

// Unary minus
template<std::floating_point M, std::integral E>
inline floatexp<M, E>
floatexp<M, E>::operator-() const {
    floatexp r(*this);
    r.negate();
    return r;
}

} // namespace wacfrac


// eval_* functions (Boost.Multiprecision backend)

namespace wacfrac {

// Arithmetic
template<std::floating_point M, std::integral E>
inline void eval_add(floatexp<M, E>& b, const floatexp<M, E>& cb) {
    b += cb;
}

template<std::floating_point M, std::integral E>
inline void eval_subtract(floatexp<M, E>& b, const floatexp<M, E>& cb) {
    b -= cb;
}

template<std::floating_point M, std::integral E>
inline void eval_multiply(floatexp<M, E>& b, const floatexp<M, E>& cb) {
    b *= cb;
}

template<std::floating_point M, std::integral E>
inline void eval_divide(floatexp<M, E>& b, const floatexp<M, E>& cb) {
    b /= cb;
}

// Conversion

namespace detail {

template<std::floating_point M, std::integral E, typename T>
void convert_to_impl(T* pa, const floatexp<M, E>& cb) {
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
inline void eval_convert_to(T* pa, const floatexp<M, E>& cb) {
    detail::convert_to_impl(pa, cb);
}

template<std::floating_point M, std::integral E, std::integral T>
inline void eval_convert_to(T* pa, const floatexp<M, E>& cb) {
    long double tmp;
    detail::convert_to_impl(&tmp, cb);
    *pa = static_cast<T>(tmp);
}

// Frexp
template<std::floating_point M, std::integral E>
inline void eval_frexp(floatexp<M, E>& b, const floatexp<M, E>& cb, E* pexp) {
    if (cb.mantissa == 0) {
        b.mantissa = 0;
        b.exponent = 0;
        *pexp = 0;
        return;
    }
    int extra;
    b.mantissa = std::frexp(cb.mantissa, &extra);
    b.exponent = 0;
    *pexp = cb.exponent + extra;
}

template<std::floating_point M, std::integral E>
inline void eval_frexp(floatexp<M, E>& b, const floatexp<M, E>& cb, int* pi) {
    if (cb.mantissa == 0) {
        b.mantissa = 0;
        b.exponent = 0;
        *pi = 0;
        return;
    }
    int extra;
    b.mantissa = std::frexp(cb.mantissa, &extra);
    b.exponent = 0;
    long long sum = static_cast<long long>(cb.exponent) + extra;
    if (sum < std::numeric_limits<int>::min() || sum > std::numeric_limits<int>::max())
        throw std::runtime_error("Exponent too large for int");
    *pi = static_cast<int>(sum);
}

// Ldexp
template<std::floating_point M, std::integral E>
inline void eval_ldexp(floatexp<M, E>& b, const floatexp<M, E>& cb, E exp) {
    b = cb;
    b.exponent += exp;
}

template<std::floating_point M, std::integral E>
inline void eval_ldexp(floatexp<M, E>& b, const floatexp<M, E>& cb, int i) {
    b = cb;
    b.exponent += i;
}

// Floor
template<std::floating_point M, std::integral E>
inline void eval_floor(floatexp<M, E>& b, const floatexp<M, E>& cb) {
    if (cb.mantissa == 0) { b.mantissa = 0; b.exponent = 0; return; }
    long double v;
    eval_convert_to(&v, cb);
    long double f = std::floor(v);
    b = f;
}

// Ceil
template<std::floating_point M, std::integral E>
inline void eval_ceil(floatexp<M, E>& b, const floatexp<M, E>& cb) {
    if (cb.mantissa == 0) { b.mantissa = 0; b.exponent = 0; return; }
    long double v;
    eval_convert_to(&v, cb);
    long double c = std::ceil(v);
    b = c;
}

// Sqrt
template<std::floating_point M, std::integral E>
inline void eval_sqrt(floatexp<M, E>& b, const floatexp<M, E>& cb) {
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

// Binary operators
#define APPLY_X_TO_OPS \
    X(+) \
    X(-) \
    X(*) \
    X(/)

#define X(OP) \
    template<std::floating_point M, std::integral E> \
    inline floatexp<M, E> \
    operator OP (const floatexp<M, E>& a, const floatexp<M, E>& b) { \
        floatexp<M, E> r(a); \
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
    inline bool operator OP (const floatexp<M, E>& a, const floatexp<M, E>& b) { \
        return a.compare(b) OP 0; \
    }
APPLY_X_TO_OPS
#undef X
#undef APPLY_X_TO_OPS

// Stream operators

template<std::floating_point M, std::integral E>
inline std::ostream& operator<<(std::ostream& os, const floatexp<M, E>& v) {
    os << v.str(os.precision(), os.flags());
    return os;
}

template<std::floating_point M, std::integral E>
inline std::istream& operator>>(std::istream& is, floatexp<M, E>& v) {
    std::string s;
    is >> s;
    v = s.c_str();
    return is;
}

} // namespace wacfrac

// number_category — explicitly in ::boost::multiprecision
namespace boost::multiprecision {

template <class Backend>
struct number_category;

template<std::floating_point M, std::integral E>
struct number_category<wacfrac::floatexp<M, E>>
    : std::integral_constant<int, 1> {};

} // namespace boost::multiprecision

namespace wacfrac {

// Aliases

using singleexp = floatexp<float, int64_t>;
using doubleexp = floatexp<double, int64_t>;
// using quadexp   = floatexp<long double, int64_t>; Unused

} // namespace wacfrac
