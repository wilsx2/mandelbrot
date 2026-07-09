// https://philthompson.me/2023/Faster-Mandelbrot-Set-Rendering-with-BLA-Bivariate-Linear-Approximation.html
#include "wacfrac/types.hpp"
#include <wacfrac/bla.tpp>

template class wacfrac::BivariateLinearApproximator<std::complex<float>>;
template class wacfrac::BivariateLinearApproximator<std::complex<double>>;
template class wacfrac::BivariateLinearApproximator<wacfrac::DoubleExpComplex>;
