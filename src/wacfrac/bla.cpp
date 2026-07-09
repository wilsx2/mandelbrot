// https://philthompson.me/2023/Faster-Mandelbrot-Set-Rendering-with-BLA-Bivariate-Linear-Approximation.html
#include "wacfrac/types.hpp"
#include <wacfrac/bla.tpp>

template class wacfrac::BivariateLinearApproximator<wacfrac::SingleComplex>;
template class wacfrac::BivariateLinearApproximator<wacfrac::DoubleComplex>;
template class wacfrac::BivariateLinearApproximator<wacfrac::DoubleExpComplex>;
