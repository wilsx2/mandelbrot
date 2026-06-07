#pragma once

#include <boost/multiprecision/mpc.hpp>
#include <boost/multiprecision/gmp.hpp>

namespace wacfrac
{

using multi_float   = boost::multiprecision::mpfr_float;
using multi_complex = boost::multiprecision::mpc_complex;

}   // namespace wacfrac
