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
using singleexp_complex = boost::multiprecision::complex_adaptor<singleexp>;
using doubleexp_complex = boost::multiprecision::number<boost::multiprecision::backends::complex_adaptor<doubleexp>>;
// using quadexp_complex   = boost::multiprecision::complex_adaptor<quadexp>; Unused

}   // namespace wacfrac
